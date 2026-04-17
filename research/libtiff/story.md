# 步长的代价：AI如何发现LibTIFF二十年老代码里的堆溢出

*CyberAI 漏洞研究叙事报告 · 2026年4月*

---

## 一

LibTIFF处理了全世界大量的专业图像。

扫描仪驱动、卫星图像管线、医疗成像系统、电影和VFX后期制作工具——当这些系统需要读写TIFF文件时，大多数最终会调用到同一个代码库：LibTIFF。这个库已经存在超过30年，是C语言中图像处理领域最古老、使用最广泛的开源库之一。

我们把它喂给了AI。

---

## 二

这次扫描的目标是LibTIFF 4.7.0的编解码器路径——PixarLog、LogLuv、ZIP/zlib。这些是处理高质量影像的压缩格式：PixarLog专为Pixar的电影制作管线设计，用对数编码保存16位像素，再用zlib压缩。

GLM-5.1收到第六段代码：`tif_pixarlog.c`的`PixarLogDecode`函数后半部分。1832字节。95秒后，模型输出了一个CRITICAL级别的发现：

> **[CRITICAL] Heap-based buffer overflow in PixarLogDecode via PIXARLOGDATAFMT_8BITABGR (L978, conf=95%)**

置信度：95%。

在Mosquitto扫描中，置信度95%的那个CRITICAL发现是错的。这次我们要仔细验证。

---

## 三

漏洞藏在一个switch语句里。

PixarLog解码支持多种输出格式：FLOAT、16BIT、8BIT……以及一个名为`8BITABGR`的格式——ABGR是Alpha、Blue、Green、Red的缩写，四字节每像素。

```c
for (i = 0; i < nsamples; i += llen, up += llen)
{
    switch (sp->user_datafmt)
    {
        case PIXARLOGDATAFMT_8BIT:
            horizontalAccumulate8(up, llen, sp->stride,
                                  (unsigned char *)op, sp->ToLinear8);
            op += llen * sizeof(unsigned char);    // 正确：op前进 llen 字节
            break;
        case PIXARLOGDATAFMT_8BITABGR:
            horizontalAccumulate8abgr(up, llen, sp->stride,
                                      (unsigned char *)op, sp->ToLinear8);
            op += llen * sizeof(unsigned char);    // ← 错误！
            break;
    }
}
```

问题在最后一行。

---

## 四

`8BIT`格式：每个输入样本（uint16_t）对应一个输出字节。`llen`个输入样本 → `llen`个输出字节。步长正确。

`8BITABGR`格式：这是ABGR，四通道。当图像是三通道RGB时（`stride=3`），`llen = 3 × imagewidth`个输入样本，输出是`4 × imagewidth`个字节（每像素添加了alpha=0通道）。

`horizontalAccumulate8abgr`函数写了`4 × W`字节。

但`op`只前进了`llen = 3 × W`字节。

两者之差：每行少前进`W`字节。

---

## 五

我们写了一个金丝雀（canary）POC，在输出缓冲区末尾放置特征字节0xDE，调用解码后检查金丝雀是否被覆盖。

关键细节：`TIFFReadEncodedStrip`在调用解码器之前，会把用户提供的缓冲区大小裁剪为`TIFFStripSize()`——即图像元数据里的"原生"条带大小（对于8位RGB为`stride × W × H`字节）。因此`PixarLogDecode`收到的`occ = stride × W × H`，而非`4 × W × H`。

```c
// poc_canary2.c（核心逻辑）
tmsize_t strip_sz = TIFFStripSize(tif);       // = 3 × W × H（按RGB计算）
unsigned char *buf    = malloc(strip_sz + CANARY);
unsigned char *canary = buf + strip_sz;
memset(canary, 0xDE, CANARY);

TIFFReadEncodedStrip(tif, 0, buf, -1);        // ← 触发漏洞
```

实际溢出计算（以100×10图像为例）：

| 量 | 计算 | 值 |
|---|---|---|
| `occ`（解码器收到） | `stride×W×H = 3×100×10` | 3000字节 |
| `llen` | `stride×W = 3×100` | 300 |
| 循环次数 | `occ/llen = H` | 10次 |
| 最后一次写入 | `buf[2700..3099]`（写400字节） | — |
| 缓冲区末尾 | `buf[2999]` | — |
| **溢出量** | **`4×W - 3×W = W`字节** | **100字节** |

结果（真实运行输出）：
```
100x10  strip_sz=3000  abgr_written=4000  expected_overflow=1000
OVERFLOW! 100 canary bytes corrupted [abgr=YES]
canary[0..15] = 00 C0 80 40 00 C0 80 40 00 C0 80 40 00 C0 80 40
```

金丝雀被实际的像素数据（A=0x00, B=0xC0, G=0x80, R=0x40）覆盖，证实了ABGR解码路径确实在运行并溢出。

溢出量恒为**W字节**（图像宽度）。对于1920×1080的高清帧，每条带溢出1920字节。

---

## 六

这是一个经典的步长错误（stride accounting bug）。

`8BITABGR`和`8BIT`复制了同一行`op += llen`，但语义不同：

- `8BIT`：每个输入样本 → 1字节输出。步长 = `llen`。✓
- `8BITABGR`：每3个输入样本 → 4字节输出（添加alpha）。步长应该是 `(llen / stride) * 4`，即 `imagewidth * 4`。✗

修复只需要改一行：

```c
// 修复前：
op += llen * sizeof(unsigned char);      // 3*W 字节

// 修复后：
op += (llen / sp->stride) * 4 * sizeof(unsigned char);   // 4*W 字节
```

一行代码，一个二十年的库，等待着某个加载了`8BITABGR`格式的PixarLog TIFF文件。

---

## 七

与此同时，AI报告了多个置信度更高的CRITICAL发现，都是错的。

**LogLuvDecode24 CRITICAL (95%)**：AI认为当`SGILOGDATAFMT_RAW`模式时，写uint32_t到一个三字节缓冲区会溢出。实际上，RAW模式下`pixel_size = sizeof(uint32_t) = 4`，所以`npixels = occ / 4`，写入的字节数恰好等于缓冲区大小。

**ZIPEncode-B CRITICAL (95%)**：AI发现`tif_rawdatasize`被强制转换为`uint64_t`后判断是否超出32位范围，认为这是"负数导致的堆溢出"。实际上这是标准的"zlib的avail字段只有32位"兼容代码，大缓冲区分多次处理。

**PixarLogDecode-A CRITICAL (92%)**：AI认为`llen`计算会整数溢出。实际上前面有明确的`rowsperstrip * llen`越界检查。

高置信度不等于正确。这是这次扫描中反复出现的教训：GLM-5.1在95%置信度下错了三次，在95%置信度下对了一次（PixarLog ABGR）。

---

## 八

这是ABGR的故事，也是步长的故事。

在图像处理代码里，`stride`是一个无处不在的概念：每行占多少字节，每个像素占多少通道，输入和输出的步长是否匹配。错误通常很小——少乘一个系数，少前进几个字节——但累积效果是每行都多写一点，最终冲出缓冲区边界。

LibTIFF的PixarLog ABGR bug就是这样。代码里两个格式放在同一个switch里，共享了相同的步长计算，但`8BITABGR`的输出比`8BIT`宽了三分之一。

AI注意到了这个不对称。人类reviewers可能因为两个case太相似而略过了它。

---

## 九

修复建议（一行代码）：

```c
// tif_pixarlog.c — PixarLogDecode 函数
case PIXARLOGDATAFMT_8BITABGR:
    horizontalAccumulate8abgr(up, llen, sp->stride,
                              (unsigned char *)op, sp->ToLinear8);
    // FIX: advance by 4 bytes per pixel (ABGR), not 1 byte per sample
    op += (llen / sp->stride) * 4 * sizeof(unsigned char);
    break;
```

---

## 十

我们写了一个金丝雀POC（`poc_canary2.c`）来确认这个漏洞：在输出缓冲区末尾放置256字节的`0xDE`标记，然后调用`TIFFReadEncodedStrip`，检查标记是否被覆盖。

```
100x10  strip_sz=3000  expected_overflow=1000
OVERFLOW! 100 canary bytes corrupted [abgr=YES]
canary[0..15] = 00 C0 80 40 00 C0 80 40 00 C0 80 40 00 C0 80 40
```

六种尺寸全部确认溢出：4×3, 4×4, 4×10, 100×10, 100×12, 100×3。溢出内容是真实的ABGR像素数据——`0x00 0xC0 0x80 0x40`对应写入时的`R=0x40, G=0x80, B=0xC0, A=0`。

这不是理论漏洞。

---

*CyberAI — 自动化漏洞研究管线*  
*扫描时间：~25分钟 · 总计发现：19个原始 → 1个HIGH确认（POC验证） · 模型：GLM-5.1*  
*POC文件：`research/libtiff/poc_canary2.c` · 构建：`cc -g -I/tmp/tiff-4.7.0/libtiff -o poc poc_canary2.c /tmp/tiff-4.7.0/libtiff/.libs/libtiff.a -lz`*
