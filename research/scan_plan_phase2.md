# CyberAI Phase 2 扫描计划

**创建日期**: 2026-04-18  
**目标**: 6 个高危场景的深度漏洞扫描  
**扫描引擎**: GLM-5.1 (Carlini Pipeline A)

---

## 目标概览与威胁场景

| # | 目标 | 版本 | 威胁场景 | 影响 | 优先级 |
|---|------|------|---------|------|--------|
| 1 | Linux kernel / eBPF + cgroup | 6.12 (master) | 容器逃逸 (Agent 跑出来) | CVSS 9+ | 🔴 P0 |
| 2 | Linux kernel / KVM | 6.12 (master) | 虚拟机逃逸 | CVSS 9+ | 🔴 P0 |
| 3 | Linux kernel / USB HID | 6.12 (master) | 连上设备就崩 (probe RCE) | CVSS 8+ | 🔴 P1 |
| 4 | ImageMagick | 7.1.2-19 | 越界读/写 — 打开文件就崩 | CVSS 7-9 | 🟠 P1 |
| 5 | Poppler | 26.04.0 | PDF 解析 DoS / RCE | CVSS 7-8 | 🟠 P2 |
| 6 | libxml2 | 2.13.5 | 供应链 — 嵌入第三方软件 | CVSS 7-9 | 🟡 P2 |
| 7 | LibTIFF | 4.7.0 | 供应链 — 图像处理管线 | CVSS 6-8 | 🟡 P2 |
| 8 | Mosquitto | 2.0.21 | 物联网设备接管 (CAND-008 已有) | CVSS 5-8 | 🟡 P3 |

---

## 攻击面分析

### 1. Linux 内核 / eBPF 容器逃逸

**攻击场景**: 容器内低权限进程 → 加载恶意 eBPF 程序 → 绕过 verifier → 内核任意代码执行 → 逃逸

**关键文件** (GLM 扫描优先级):

| 文件 | 行数 | 关注点 |
|------|------|--------|
| `kernel/bpf/verifier.c` | ~15K | JIT verifier bounds checking, register tracking |
| `kernel/bpf/syscall.c` | ~5K | BPF_PROG_LOAD, map操作权限检查 |
| `kernel/bpf/core.c` | ~3K | BPF 解释器执行 |
| `kernel/bpf/btf.c` | ~8K | BTF 类型验证 |
| `kernel/cgroup/cgroup.c` | ~5K | cgroup namespace 隔离 |

**漏洞模式**:
- `verifier.c` 中 register 范围跟踪不精确 → OOB map 访问
- BPF map 类型混淆
- speculative execution bypass

### 2. Linux 内核 / KVM 虚拟机逃逸

**攻击场景**: Guest OS → 触发 KVM hypercall → Host kernel 越界访问 → 逃逸

**关键文件**:

| 文件 | 行数 | 关注点 |
|------|------|--------|
| `arch/x86/kvm/emulate.c` | ~6K | x86 指令模拟 — 极复杂 |
| `arch/x86/kvm/hyperv.c` | ~3K | Hyper-V 兼容层 |
| `arch/x86/kvm/svm/svm.c` | ~4K | AMD SVM 处理 |
| `virt/kvm/kvm_main.c` | ~5K | KVM ioctl 接口 |
| `arch/x86/kvm/mmu/mmu.c` | ~6K | 内存管理单元模拟 |

### 3. Linux 内核 / USB-HID 驱动崩溃

**攻击场景**: 插入恶意 USB 设备 → 内核 probe() → 解析畸形 descriptor → 内存越界

**关键文件**:

| 文件 | 行数 | 关注点 |
|------|------|--------|
| `drivers/usb/core/config.c` | ~1.2K | USB 配置描述符解析 ← 主要 |
| `drivers/usb/core/hub.c` | ~4K | Hub 枚举和 probe |
| `drivers/hid/hid-core.c` | ~2K | HID 描述符解析 |
| `drivers/usb/core/driver.c` | ~1K | 驱动绑定 |

### 4. ImageMagick 7.1.2-19 越界读/写

**攻击场景**: `convert malicious.tiff output.png` → 崩溃/任意代码执行

**关键文件** (按危险度排序):

| 文件 | 关注点 |
|------|--------|
| `coders/tiff.c` | TIFF decode — 历史上 CVE 最多 |
| `coders/png.c` | PNG chunk 处理 |
| `coders/jxl.c` | JPEG XL (新格式，代码年轻) |
| `coders/heic.c` | HEIC/HEIF |
| `coders/svg.c` | SVG — libxml2 攻击面 |
| `coders/pdf.c` | PDF 嵌入图像 |
| `MagickCore/pixel.c` | 像素量化/转换 |
| `MagickCore/quantum-import.c` | 外部数据导入 |

### 5. Poppler 26.04.0 PDF 解析 DoS

**攻击场景**: 打开恶意 PDF → 内存耗尽 / 无限循环 / 崩溃

**关键文件**:

| 文件 | 行数 | 关注点 |
|------|------|--------|
| `poppler/JBIG2Stream.cc` | 4284 | JBIG2 图像解码 — 历史高危 |
| `poppler/JPXStream.cc` | 3104 | JPEG 2000 流 |
| `poppler/Stream.cc` | 5340 | 通用流处理 — 整数溢出 |
| `poppler/XRef.cc` | 2001 | XRef 表解析 — 无限循环 |
| `poppler/GfxFont.cc` | 2421 | 字体处理 |
| `poppler/Parser.cc` | ~800 | PDF 对象解析 |

### 6. libxml2 2.13.5 供应链

**关注点**: libxml2 被嵌入大量软件 (PHP, Python, Ruby, Node.js 等)
**攻击场景**: 恶意 XML/HTML → 通过宿主软件执行

已有 32 个 extracts，直接开始扫描。

### 7. LibTIFF 4.7.0 供应链

**关注点**: CAND-001/002 已为 FP，需要更深入扫描 PixarLog/LZW/ZIP 编解码路径。
已有 extracts，补充扫描。

### 8. Mosquitto 2.0.21 物联网接管

CAND-008 (proplen underflow) 已完成，继续寻找：
- SUBSCRIBE/UNSUBSCRIBE 属性解析
- RETAIN 消息存储
- ACL bypass
- 认证绕过

---

## 执行计划

### Phase 2a — 立即执行 (高优先级)

```
[T+0h]  创建 Linux kernel extracts (eBPF/KVM/USB)
[T+1h]  创建 ImageMagick 7.1.2-19 extracts
[T+1h]  创建 Poppler 26.04.0 extracts
[T+2h]  启动 libxml2 GLM 扫描 (已有 extracts)
[T+2h]  启动 Linux kernel GLM 扫描
[T+3h]  启动 ImageMagick 扫描
[T+3h]  启动 Poppler 扫描
[T+6h]  收集扫描结果，开始 triage
```

### Phase 2b — 深度验证 (找到候选后)

- eBPF/KVM 漏洞：在 QEMU + KVM 环境下验证
- USB 驱动：使用 USB gadget 模拟恶意设备
- ImageMagick：创建 PoC 文件触发 crash
- Poppler：创建 PoC PDF

---

## 扫描参数

```python
GLM_MODEL = "glm-4-flash"  # 速度优先
CONFIDENCE_THRESHOLD = 75
SEGMENT_MAX_CHARS = 5000
TIMEOUT_PER_CALL = 150
INTER_CALL_DELAY = 20  # 秒
```

---

## 已知 CVE 排除列表 (避免重复发现)

### eBPF 近期 CVE
- CVE-2021-3490: speculative OOB (fixed 5.11)
- CVE-2021-31440: verifier bypass (fixed 5.12)
- CVE-2022-0500: verifier check bypass
- CVE-2023-2163: BPF pointer arithmetic (fixed 6.3)

### KVM 近期 CVE
- CVE-2022-0330: TLB flush missing
- CVE-2022-45869: x86 KVM shadow paging
- CVE-2024-26581: nft_set_rbtree OOB

### ImageMagick 近期 CVE
- CVE-2025-62171: BMP number_colors (already known, 7.1.1-44)
- CVE-2025-57803: BMP bytes_per_line (already known)

### Poppler 近期 CVE
- CVE-2022-38784: JBIG2 integer overflow
- CVE-2023-34872: OOB read in PSOutputDev
