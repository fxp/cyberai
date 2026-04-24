# 用 LLM 扫描 1328 个漏洞发现：90% 是误报，但我们学到了什么

**作者**: CyberAI Security Research  
**日期**: 2026-04-20  
**项目**: [CyberAI](https://fxp.github.io/cyberai/) — 大模型驱动的自动化漏洞发现平台

---

## 一句话版本

我们用 GLM-4-flash 对 Linux 内核（BPF/KVM/USB）、ImageMagick、Poppler、libxml2 六个目标扫描了 1328 个疑似漏洞，手动验证后全部是误报。但这次失败让我们搞清楚了"为什么"，并且发现了正确的做法。

---

## 背景：CyberAI 是什么

CyberAI 是一个用大语言模型自动发现软件漏洞的实验平台。核心思路来自 Nicholas Carlini 2023 年的一篇博文——用 LLM 替代人类安全研究员做代码审计，实现漏洞发现的规模化。

我们的 Pipeline A（Carlini 管线）工作方式很简单：

1. 把目标项目的 C/C++ 源文件切成 5000 字符的片段（extract）
2. 每个 extract 送给 GLM-4-flash，提问："这段代码里有没有内存安全漏洞？"
3. 收集置信度 ≥ 90% 的 findings，去重，排序
4. 人工复核高优先级候选

Phase 1（4月初）我们就这样找到了 Mosquitto MQTT broker 的一个真实漏洞（`property_length` uint32_t 下溢），已于 4月18日提交披露。

Phase 2（本次）我们把目标扩大了 26 倍。

---

## Phase 2：规模扩大，结果难看

### 扫描范围

| 目标 | 版本 | 扫描 extracts | Findings | CRITICAL |
|------|------|---------------|----------|----------|
| Linux BPF (eBPF 验证器) | 6.12 | 325 | 260 | 0 |
| Linux KVM (虚拟机监控) | 6.12 | 264 | 204 | 17 |
| Linux USB/HID 驱动 | 6.12 | 119 | 110 | 17 |
| ImageMagick | 7.1.2-19 | 514 | 431 | 0 |
| Poppler (PDF 解析) | 26.04.0 | 359 | 294 | 3 |
| libxml2 | 2.13.5 | ~200 | 29 | 0 |
| **合计** | | **~1780** | **1328** | **37** |

数字很漂亮，但结果很难看：**37 个 CRITICAL，手动复核后全部是误报。**

---

## 为什么全是误报

### 根因：5000 字符的上下文窗口

Pipeline A 的致命缺陷：它看到的每个代码片段只有 5000 个字符，而边界检查往往在相邻函数里。

举几个典型例子：

**linux_kvm：`arch_x86_kvm_hyperv_P.c`**

GLM 说：`kvm_hv_flush_tlb()` 没有检查 `rep_idx`/`rep_cnt` 就使用了。

实际上，代码的前一个 extract 里有：
```c
if (unlikely(!hc.rep_cnt || hc.rep_idx))
    return HV_STATUS_INVALID_HYPERCALL_INPUT;
```
模型没看到这行，就报了 CRITICAL。

**linux_usb：`drivers_usb_core_hub_K.c`**

GLM 说：`hub->buffer` 没有边界检查，可能 OOB 写。

实际代码第 74 行：
```c
if (maxp > sizeof(*hub->buffer))
    maxp = sizeof(*hub->buffer);
```
这行就在同一个文件里，只是在 5000 字符窗口的边缘之外。

**poppler：`GfxStatec_N.c`**

GLM 说：析构函数 `delete path` 没有 null 检查，Use-After-Free。

事实：在 C++ 中，`delete nullptr` 是完全合法的 no-op，这是语言标准保证的行为，不是漏洞。

**linux_kvm：`arch_x86_kvm_emulate_F.c`**

这个最有趣。二次验证用更严格的 prompt 跑了一遍，模型仍然 "CONFIRMED"，理由是"`linearize()` 被以 size=0 调用，绕过了边界检查"。

我去读了原始代码：
```c
rc = linearize(ctxt, addr, size, true, &linear);
```
`size` 就是函数参数，不是字面量 0。模型在阅读代码时幻觉了。

---

## 二次验证：仍然不够

面对 1328 个 findings，我们写了一个二次验证脚本：

- 对每个 extract 加载前后各一段代码（最多 8000 字符）
- 换一个"怀疑论者"提示词——"你的任务是驳倒这个漏洞"
- 只取前 30 个高置信度候选

结果：30 个候选，27 个 FALSE_POSITIVE，3 个 "CONFIRMED"。

然后我手动复核了这 3 个：

| 文件 | GLM CONFIRMED 理由 | 手动结论 |
|------|-------------------|---------|
| `arch_x86_kvm_emulate_F.c` | linearize 以 size=0 调用 | **FP** — 模型幻觉，size 是参数变量 |
| `poppler/GfxStatec_N.c` | delete path 无 null 检查 | **FP** — C++ 标准：delete nullptr 合法 |
| `poppler/GfxStatec_\`.c` | gmallocn(nVertices) 无溢出保护 | **FP** — gmallocn 内部有 checkedMultiply + abort() |

最后一个最有迷惑性。`GfxGouraudTriangleShading` 拷贝构造函数：

```cpp
vertices = (GfxGouraudVertex *)gmallocn(nVertices, sizeof(GfxGouraudVertex));
```

看起来 `nVertices` 未经验证，可能整数溢出。但读完 `gmem.h`：

```cpp
inline void *gmallocn(int count, int size, bool checkoverflow = false) {
    if (count == 0) return nullptr;
    int bytes;
    if (count < 0 || size <= 0 || checkedMultiply(count, size, &bytes)) {
        std::fputs("Bogus memory allocation size\n", stderr);
        std::abort();   // ← 不是堆破坏，是 abort
    }
    return gmalloc(bytes, checkoverflow);
}
```

溢出 → abort，不是任意写。加上触发溢出需要 ~715M 个三角形（~8.6 GB 数据），PDF 解析根本不可能走到这里。

**Phase 2 最终结果：0 个确认漏洞。**

---

## 三个真实原因

复盘下来，Pipeline A 的 90% 误报率不是模型能力差，而是工程设计问题：

### 1. 上下文窗口太小

5000 字符 ≈ 100-150 行代码。一个函数的防御检查经常在调用方，或者在头文件、相邻函数里。模型只看到局部，看不到全局。

**修复方向**：Pipeline B — 整文件扫描，或者至少加载整个函数 + 所有调用方。

### 2. 模型对"不存在的代码"产生幻觉

这是 LLM 用于代码审计时最危险的特性：模型不会说"我不知道"，它会用它"以为"的代码来推理。`linearize(ctxt, addr, size, ...)` 里的 `size` 被幻觉成字面量 0，产生出一条完整的漏洞分析。

**修复方向**：让模型先"抄写"它在分析的具体代码行，再给出结论。强迫 grounding。

### 3. 置信度数字没有意义

GLM 报告 confidence=95，这只是模型的主观自评，和实际真实率毫无关系。我们跑的 30 个 confidence≥90 的候选，真实率是 0%。

**修复方向**：用 N-day CVE 数据集做 calibration。

---

## 那 Phase 1 的 Mosquitto 漏洞是怎么找到的

一个好问题。

Mosquitto 的 `property__read()` 漏洞能被找到，是因为它的触发条件非常集中：

```c
// property__read_all():
while (proplen > 0) {
    property__read(packet, &proplen, p);  // proplen 1 → 0 → 0xFFFFFFFF
}
```

漏洞代码本身（uint32_t 下溢 + 循环继续）在一个 extract 里完整可见。不需要看相邻代码。这是 Pipeline A 的"甜点"：**单函数内的逻辑漏洞**。

反观 Phase 2 的目标（Linux 内核、Poppler、ImageMagick），它们是经过多年严格 review 的成熟项目，防御检查大量分散在调用链的不同层次。Pipeline A 在这些目标上几乎必然失败。

---

## 下一步：Pipeline B

基于这次经验，我们正在设计 Pipeline B：

```
目标项目
    │
    ▼
符号分析 (tree-sitter / clang)
    │  函数边界 + 调用图
    ▼
以函数为单位，加载调用方 + 被调用方
    │  上下文 ~20,000 字符
    ▼
Claude / GLM 全函数审计
    │
    ▼
PoC 生成 + 容器化验证
    │
    ▼
自动崩溃 → confirmed
```

关键改进：
- **函数级上下文**而非固定字符窗口
- **调用图感知**——加载直接调用方，模型能看到参数来源
- **容器化 PoC 验证**——让程序自己说话，不依赖模型判断

---

## 数字对比

| | Phase 1 | Phase 2 |
|---|---|---|
| 目标数 | 5 (Mosquitto/OpenSSL等) | 6 (Linux/IM/Poppler/libxml2) |
| Findings | ~50 | 1,328 |
| 确认漏洞 | 1 (CAND-008, 已披露) | 0 |
| 手动工时 | ~4h | ~12h |
| 误报率 | ~98% | ~100% |
| 最大收获 | Mosquitto DoS CVE 候选 | 误报根因分析 + Pipeline B 设计 |

---

## 结语

这次扫描的数字看起来很好看——1328 个 findings，37 个 CRITICAL——但背后是零个真实漏洞。

这不是失败，这是校准。

我们现在知道：
- Pipeline A 的边界在哪里（单函数可见的逻辑漏洞）
- 误报的根本原因（上下文窗口 + 模型幻觉 + 没有运行验证）
- 下一步该做什么（Pipeline B + 容器化 PoC）

LLM 辅助漏洞研究是真实可行的，Mosquitto 证明了这一点。扩大规模需要更好的工程，不只是更多的 API 调用。

---

*CyberAI 平台: [fxp.github.io/cyberai](https://fxp.github.io/cyberai/)*  
*CAND-008 (Mosquitto) 披露草稿: `research/disclosures/mosquitto_CAND-008_draft.md`*  
*Phase 2 triage 报告: `research/triage_report_phase2.md`*
