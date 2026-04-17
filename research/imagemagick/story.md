# 无限解码：AI在ImageMagick里找到的两个古老DoS

*CyberAI 漏洞研究叙事报告 · 2026年4月*

---

## 一

ImageMagick是图像处理领域的瑞士军刀。

它运行在web服务器上，为用户上传的图片生成缩略图。它被嵌入到CI/CD流水线里，在部署前处理资产。它是无数个"convert"命令背后的引擎，从PDF渲染到批量格式转换，ImageMagick的coders目录处理了几十种图像格式，每一种都是一个潜在的攻击面。

我们从最经典的两种格式开始：GIF和BMP。

---

## 二

第一个发现来自GIF解码器里一个三行的验证函数。

```c
data_size=(unsigned char) ReadBlobByte(image);
if (data_size > MaximumLZWBits)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
lzw_info=AcquireLZWInfo(image,data_size);
```

GLM-5.1扫描这段代码花了444秒，给出了一个HIGH级别的发现：

> **[HIGH] Missing minimum LZW data_size validation allows decoder malfunction (L427, conf=75%)**

`data_size`来自文件，只检查了上界（不超过12位），没有检查下界。

GIF规范要求最小码长为2。`data_size=0`时会发生什么？

---

## 三

追踪数据流：

- `maximum_data_value = (1 << 0) - 1 = 0`（只有一个有效码：0）
- `clear_code = 1, end_code = 2`
- `bits = 1, maximum_code = 2`（只能读到1位的码）

解码器每次读1位，能读到的值只有0和1。`clear_code=1`是可读的——它触发解码器重置。`end_code=2`需要2位表示，但解码器只读1位，永远不会看到它。

结果：解码器重置，重置，再重置……直到读完所有压缩数据子块，然后等待更多数据。

一个攻击者可以构造一个充满大子块（每个255字节）的GIF文件，每个子块都不包含结束码，只包含触发重置的clear_code。解码器会消耗大量CPU时间处理这些字节，然后继续等待。

这是一个DoS。不是内存损坏，是CPU时间损耗。

修复只需要加一个条件：
```c
if (data_size < 2 || data_size > MaximumLZWBits)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
```

---

## 四

第二个发现来自BMP文件头解析。

OS/2的BMP格式支持"Bitmap Array"（BA）——多张BMP图像打包在一个文件里，每张图前面有一个14字节的BA头，指向下一个BA头或图像数据。

ImageMagick的解析循环：

```c
while (LocaleNCompare((char *) magick,"BA",2) == 0)
{
    bmp_info.file_size=ReadBlobLSBLong(image);
    bmp_info.ba_offset=ReadBlobLSBLong(image);
    bmp_info.offset_bits=ReadBlobLSBLong(image);
    count=ReadBlob(image,2,magick);
    if (count != 2)
        break;
}
```

没有循环次数限制。

每次循环读取14字节，线性前进。没有跳转（`ba_offset`被读取但从不使用来seek）。攻击者可以构造一个1GB的文件，充满连续的BA头，触发约7100万次循环迭代——每次都解析14字节，绑定解析线程的CPU。

文件处理完毕之前，CPU一直在全速消耗。

这不是一个理论漏洞。任何调用`magick convert`处理用户提交文件的web服务都面临这个风险。

---

## 五

与此同时，AI报告了大量更高置信度的CRITICAL发现，几乎全部是错的。

**CRITICAL 85%：number_colors整数溢出**

```c
bmp_info.number_colors=ReadBlobLSBLong(image);
// ...后面有30行代码...
```

模型看到了"未经验证的字段直接用于后续分配"的模式。但它没有看到代码在30行之后的验证：

```c
if ((bmp_info.bits_per_pixel < 16) &&
    (bmp_info.number_colors > (1U << bmp_info.bits_per_pixel)))
    ThrowReaderException(CorruptImageError,"UnrecognizedNumberOfColors");
if ((MagickSizeType) bmp_info.number_colors > blob_size)
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
```

验证在提取窗口之外。这是这次扫描最常见的假阳性模式：模型看到危险的赋值，但看不到后面的校验，因为校验代码在提取边界之外。

**HIGH 90%：OS/2 BMP维度验证缺失**

模型认为`bmp_info.width`读取后没有验证就使用。实际上后面有：

```c
if (bmp_info.width == 0)
    ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
image->columns=(size_t) MagickAbsoluteValue(bmp_info.width);
```

负值由`MagickAbsoluteValue`处理。零值有专门的检查。

**HIGH 75%：ReadBlobBlock返回负值导致缓冲区下溢**

这个发现最迷惑人：

```c
count=ReadBlobBlock(image,buffer);
if (count == 0) break;
buffer[count]='\0';    // ← 如果count是-1，就是buffer[-1]
```

这看起来很危险。但`ReadBlobBlock`的实现在出错时总是返回0，不返回负值：

```c
static ssize_t ReadBlobBlock(Image *image,unsigned char *data)
{
    ...
    if (count != (ssize_t) block_count)
        return(0);    // ← 出错返回0，不返回-1
    return(count);
}
```

模型识别出了危险的算术模式，但没有追踪到上游函数的不变量。这是LLM安全扫描最典型的局限——和Mosquitto扫描里的payloadlen假阳性一模一样的问题。

---

## 六

这次扫描确认了一个规律：

ImageMagick的`coders/`目录设计良好，大量潜在的内存损坏向量都有防御性代码，但防御代码往往在危险赋值的下方10-50行。提取窗口为4-5KB时，这个距离恰好足以让验证代码落在边界外，导致模型产生高置信度的假阳性。

真正的漏洞藏在更微妙的地方：逻辑边界（GIF的data_size=0）和时间复杂度（BMP的BA循环）。这两种漏洞不涉及直接的内存越界，而是对解析语义的错误处理。

这也说明了为什么DoS漏洞经常被忽视：它们没有崩溃转储，没有CVE的"9.8 CRITICAL"标签，只是悄悄地把服务器的CPU吃掉，直到超时断连。

---

*CyberAI — 自动化漏洞研究管线*  
*扫描时间：~120分钟 · 总计发现：28个原始 → 2个MEDIUM确认 · 模型：GLM-5.1*
