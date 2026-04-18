#!/usr/bin/env python3
"""
Phase 2 批量创建扫描 extracts
目标: ImageMagick 7.1.2-19, Poppler 26.04.0, Linux kernel 6.12 (eBPF/KVM/USB)
"""
import os, re, sys

TARGETS_DIR = "/Users/xiaopingfeng/Projects/cyberai/targets"
EXTRACTS_DIR = "/Users/xiaopingfeng/Projects/cyberai/scripts/extracts"
MAX_CHARS = 5000

def split_c_file(filepath, label_prefix, output_dir):
    """将 C/C++ 文件按函数分割，每段 MAX_CHARS 字符以内"""
    try:
        with open(filepath, encoding='utf-8', errors='replace') as f:
            content = f.read()
    except:
        return 0

    if len(content) < 200:
        return 0

    # 按函数边界分割（简单按 \n} 加空行）
    func_pattern = re.compile(
        r'(?:^(?:static\s+)?(?:[\w\s\*]+?\s+)+\w+\s*\([^)]*\)\s*\{)',
        re.MULTILINE
    )

    basename = os.path.basename(filepath).replace('.c', '').replace('.cc', '')
    os.makedirs(output_dir, exist_ok=True)

    # 简单按大小分块
    chunks = []
    start = 0
    while start < len(content):
        end = start + MAX_CHARS
        if end < len(content):
            # 找最近的函数边界
            nl = content.rfind('\n\n', start, end)
            if nl > start + 1000:
                end = nl
        chunks.append(content[start:end])
        start = end

    count = 0
    for i, chunk in enumerate(chunks):
        if len(chunk.strip()) < 100:
            continue
        fname = f"{label_prefix}_{chr(65+i)}.c"
        out = os.path.join(output_dir, fname)
        with open(out, 'w') as f:
            f.write(f"// Source: {filepath}\n// Segment {i+1}/{len(chunks)}\n\n")
            f.write(chunk)
        count += 1
    return count

def process_target(name, base_path, file_patterns, output_subdir):
    out_dir = os.path.join(EXTRACTS_DIR, output_subdir)
    os.makedirs(out_dir, exist_ok=True)
    total = 0

    for root, dirs, files in os.walk(base_path):
        # 排除 test/ build/ 目录
        dirs[:] = [d for d in dirs if d not in ('test', 'tests', 'build', 'doc', 'docs', '.git')]
        for fname in files:
            fpath = os.path.join(root, fname)
            rel = os.path.relpath(fpath, base_path)

            matched = any(
                re.search(pat, rel, re.IGNORECASE)
                for pat in file_patterns
            )
            if not matched:
                continue

            label = re.sub(r'[/\\]', '_', rel.replace('.c', '').replace('.cc', ''))
            n = split_c_file(fpath, label, out_dir)
            if n:
                total += n
                print(f"  {rel} → {n} 段")

    print(f"\n✅ {name}: {total} 个 extracts → {out_dir}")
    return total


print("=" * 60)
print("Phase 2 Extract 生成")
print("=" * 60)

# ── 1. ImageMagick 7.1.2-19 ──────────────────────────────────
print("\n[1] ImageMagick 7.1.2-19 (OOB crash focus)")
IM_BASE = f"{TARGETS_DIR}/ImageMagick-7.1.2-19"
IM_PATTERNS = [
    r'^coders/(tiff|png|jpeg|jxl|heic|svg|pdf|webp|bmp|gif|jp2)\.c$',
    r'^MagickCore/(pixel|quantum-import|quantum-export|cache|constitute|compress)\.c$',
]
process_target("ImageMagick", IM_BASE, IM_PATTERNS, "imagemagick_712")

# ── 2. Poppler 26.04.0 ──────────────────────────────────────
print("\n[2] Poppler 26.04.0 (DoS focus)")
PP_BASE = f"{TARGETS_DIR}/poppler-26.04.0/poppler"
PP_PATTERNS = [
    r'(JBIG2Stream|JPXStream|Stream|XRef|Parser|GfxFont|GfxState|Gfx|PDFDoc|Form|Annot)\.cc$',
]
process_target("Poppler", PP_BASE, PP_PATTERNS, "poppler")

# ── 3. Linux kernel / eBPF ───────────────────────────────────
print("\n[3] Linux kernel eBPF (container escape)")
KERN_BASE = f"{TARGETS_DIR}/linux-6.12-sparse"
BPF_PATTERNS = [
    r'^kernel/bpf/(verifier|syscall|core|btf|helpers|tnum|arena)\.c$',
    r'^kernel/cgroup/cgroup\.c$',
]
process_target("Linux-eBPF", KERN_BASE, BPF_PATTERNS, "linux_bpf")

# ── 4. Linux kernel / KVM ───────────────────────────────────
print("\n[4] Linux kernel KVM (VM escape)")
KVM_PATTERNS = [
    r'^arch/x86/kvm/(emulate|hyperv|mmu|vmx/vmx|svm/svm|x86)\.c$',
    r'^virt/kvm/kvm_main\.c$',
]
process_target("Linux-KVM", KERN_BASE, KVM_PATTERNS, "linux_kvm")

# ── 5. Linux kernel / USB-HID ───────────────────────────────
print("\n[5] Linux kernel USB/HID (device crash)")
USB_PATTERNS = [
    r'^drivers/usb/core/(config|hub|driver|message|quirks)\.c$',
    r'^drivers/hid/(hid-core|hid-input|hid-generic)\.c$',
]
process_target("Linux-USB", KERN_BASE, USB_PATTERNS, "linux_usb")

print("\n🎉 Phase 2 extracts 全部生成完毕")
