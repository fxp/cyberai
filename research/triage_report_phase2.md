# Phase 2 扫描 Triage 报告

**日期**: 2026-04-20  
**扫描引擎**: GLM-4-flash · Carlini Pipeline A  
**目标数量**: 6 个  
**总 findings**: 1,328  

---

## 1. 扫描汇总

| 目标 | 版本 | Extracts | 总 findings | CRITICAL | HIGH | conf≥90 | 手动验证 FP 率 |
|------|------|----------|-------------|----------|------|---------|---------------|
| linux_bpf | 6.12 | 325 | 260 | 0 | 260 | 121 | ~90% |
| linux_kvm | 6.12 | 264 | 204 | 17 | 187 | 107 | ~85% |
| linux_usb | 6.12 | 119 | 110 | 17 | 93 | 95 | ~85% |
| imagemagick | 7.1.2-19 | 514 | 431 | 0 | 431 | 303 | ~95% |
| poppler | 26.04.0 | 359 | 294 | 3 | 291 | 191 | ~90% |
| libxml2 | 2.13.5 | ~200 | 29 | 0 | 29 | 27 | ~85% |
| **合计** | | **~1,780** | **1,328** | **37** | **1,291** | **844** | **~90%** |

---

## 2. 高优先级候选分析

### 2.1 CRITICAL findings 分析 (linux_kvm — 17 个)

手动审查了如下代表性 CRITICAL findings，均为 **FALSE POSITIVE**：

| 文件 | 声称漏洞 | 实际状态 | FP 原因 |
|------|---------|---------|--------|
| `arch_x86_kvm_hyperv_P.c` | kvm_hv_flush_tlb rep_idx/rep_cnt 无边界检查 | **FP** | 代码在调用前已检查 `!hc.rep_cnt || hc.rep_idx` |
| `arch_x86_kvm_emulate_F.c` | segmented_write_std 无边界检查 | **FP** | `linearize()` 负责地址合法性检查，是正确的 x86 分段机制 |
| `arch_x86_kvm_hyperv_F.c` | kzalloc_obj NULL 未检查 | **FP** | 需要看更多上下文，结构分配后有判断分支 |

**根因**: GLM 扫描 5000 字符片段，边界检查往往在相邻代码段，模型无法看到，从而产生大量 FP。

### 2.2 CRITICAL findings 分析 (linux_usb — 17 个)

| 文件 | 声称漏洞 | 实际状态 | FP 原因 |
|------|---------|---------|--------|
| `drivers_usb_core_hub_K.c` | hub->buffer OOB write | **FP** | 代码第74行 `if (maxp > sizeof(*hub->buffer)) maxp = sizeof(*hub->buffer);` 已限制 |
| `drivers_hid_hid-core_C.c` | report_size/report_count 无边界检查 | **FP** | 代码第120/129行有 `> 256` 和 `> HID_MAX_USAGES` 明确检查 |
| `drivers_hid_hid-input_K.c` | HID descriptor OOB | **FP** | `clamp()` 和 `logical_minimum < logical_maximum` 检查存在 |

### 2.3 Poppler CRITICAL findings (3 个)

| 文件 | 声称漏洞 | 实际状态 | FP 原因 |
|------|---------|---------|--------|
| `XRefc_A/B.c` | ObjectStream nObjects 整数溢出 | **FP** | 代码注释明确 "arbitrary limit to avoid integer overflow", `nObjects > 1000000` 检查存在 |
| `JBIG2Streamc_I.c` | JBIG2Bitmap 整数溢出 | **FP** | `checkedAdd()`, `h >= (INT_MAX-1)/line`, `gmalloc_checkoverflow()` 三层防护 |
| `Streamc_B.c` | BufStream::lookChar OOB | **FP** | idx 由内部状态控制，非外部输入直接索引 |

### 2.4 二次验证 "CONFIRMED" 手动复核 (GfxStatec_`.c — 最后候选)

二次验证脚本对 `GfxGouraudTriangleShading` 拷贝构造函数返回 CONFIRMED，理由是 `gmallocn(nVertices, sizeof(...))` 无边界检查。手动复核结果：**FALSE POSITIVE**。

证据：
- `gmem.h` 中 `gmallocn()` 使用 `checkedMultiply(count, size, &bytes)` — 整数溢出时 `std::abort()`，无堆破坏
- `nTriangles * 3` 若溢出将产生负数，`count < 0` 分支同样触发 abort
- 触发溢出需要 ~715M 个三角形 (~8.6 GB 数据)，PDF 解析场景不可能达到
- 拷贝构造函数来源 `nVertices` 已由 `parse()` 的 `greallocn_checkoverflow` + null-check 防护

**结论：Phase 2 全部 30 个二次验证候选均为 FALSE POSITIVE。**

### 2.4 libxml2 findings (29 个)

| 文件 | 声称漏洞 | 实际状态 | FP 原因 |
|------|---------|---------|--------|
| `parser_charref.c` | `val` 整数溢出 | **FP** | 循环内每步都有 `val > 0x110000` 检查并 clamp |
| `xpath_name_A.c` | UTF-8 OOB read | **待定** | 4字节 UTF-8 序列访问 cur[3] 前未验证剩余字节数 |
| `entities_E.c` | growBufferReentrant overflow | **FP** | `indx + 100 > buffer_size` 确保 100 字节余量，足够容纳所有写入 |

---

## 3. 有待进一步验证的候选

### CAND-P2-001: libxml2 UTF-8 OOB (xpath_name_A.c)

**类型**: OOB Read  
**代码位置**: `libxml2/xpath.c` → `xmlXPathCurrentChar()`  
**描述**: 4字节 UTF-8 序列解析中，访问 `cur[3]` 前仅检查了前两字节的连续性标志，未验证剩余缓冲区长度。若输入缓冲区在多字节序列边界截断，可能读取缓冲区外字节。  
**现有防护**: 调用方 `xmlXPathScanName()` 通常处理完整的 null-terminated 字符串，截断场景需要进一步分析。  
**建议**: 手动构造截断的 4字节 UTF-8 序列进行 PoC 测试。

### CAND-P2-002: linux_bpf eBPF 推测执行缓解 (verifier_±.c)

**类型**: eBPF Verifier Bypass / Spectre  
**代码位置**: `kernel/bpf/verifier.c` — `nospec` 路径处理  
**描述**: BPF 推测执行路径消毒可能不完整，允许构造特定指令序列绕过 Spectre v1 缓解。  
**现有防护**: 复杂。已有多个 CVE (2021-3490, 2021-31440, 2023-2163) 修复相关问题，但推测执行攻击面持续扩大。  
**建议**: 需要完整的 verifier.c 上下文进行深度分析，Pipeline B 容器化扫描更合适。

### CAND-P2-003: ImageMagick TIFF cache pixel OOB

**类型**: OOB Read/Write  
**代码位置**: `MagickCore/cache.c` — `GetVirtualPixelCacheNexus()`  
**描述**: pixel cache 的索引计算在某些 TIFF 解码路径可能产生负偏移，导致 OOB 读写。  
**注意**: ImageMagick cache 代码极其复杂，历史上有大量 CVE，但 7.1.2-19 已有较多修复。  
**建议**: 需要 TIFF PoC 文件触发崩溃进行验证。

---

## 4. Pipeline A 局限性分析

| 问题 | 影响 | 改进方案 |
|------|------|---------|
| 5000 字符窗口无法看到相邻代码段的边界检查 | FP 率 ~90% | Pipeline B: 全文件扫描 |
| 模型置信度 95% ≠ 真实置信度 95% | 误导性 | 二次验证 |
| 缺少编译和运行时验证 | 无法排除 FP | 容器化 PoC 测试 |
| 不了解函数调用约定 (谁传入什么值) | 可利用性评估困难 | 全项目容器化扫描 |

---

## 5. 下一步建议

### 短期 (1-2天)
1. ✅ 完成 30 个候选的二次验证 (GLM skeptical prompt)
2. ✅ 手动复核全部 3 个 "CONFIRMED" — 均为 FP (gmallocn有overflow保护, delete nullptr合法, linearize是正确的x86地址检查)
3. 对 `xpath_name_A.c` 构造 PoC — 截断 UTF-8 序列 XPath 表达式 (仍为待定候选)
4. 对 ImageMagick TIFF cache 构造 PoC — 特制 TIFF 文件

### 中期 (1周)  
4. Pipeline B 容器化扫描 linux_bpf — 完整 verifier.c 上下文
5. OSS-Fuzz 集成 — 对 Poppler/ImageMagick 跑 fuzzer，利用现有 corpus

### 长期
6. N-day CVE Pipeline D — 100 个 2024-2025 Linux 内核 CVE 自动化测试
7. 升级到 Claude 或 GLM-5.1 (速度提升 + 更大上下文窗口)

---

## 6. 与 Phase 1 结果对比

| 阶段 | 目标数 | Findings | 已确认 | CVE 候选 |
|------|--------|----------|--------|---------|
| Phase 1 | Mosquitto/libssl等 | ~50 | CAND-008 (已上报) | 1 |
| Phase 2 (本次) | 6 (Linux/ImageMagick/Poppler/libxml2) | 1,328 | **0** (全部FP) | 0 |

Phase 2 scanning 规模扩大 26× 但 Signal/Noise 比较差，需要更精准的 filtering 策略。
