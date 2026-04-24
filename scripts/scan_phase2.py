#!/usr/bin/env python3
"""
Phase 2 统一扫描脚本 — GLM-4-flash Carlini Pipeline A
用法:
  python3 scan_phase2.py --target libxml2
  python3 scan_phase2.py --target linux_bpf --confidence 85 --output-dir /tmp/results
"""
import os, sys, json, time, argparse
from pathlib import Path
from datetime import datetime

try:
    from zhipuai import ZhipuAI
    HAS_ZHIPU = True
except ImportError:
    HAS_ZHIPU = False

# ── 参数解析 ──────────────────────────────────────────────────────
parser = argparse.ArgumentParser(description="Pipeline A GLM batch scanner")
parser.add_argument("--target", required=True,
                    choices=["imagemagick","poppler","linux_bpf","linux_kvm",
                             "linux_usb","libxml2","libtiff"],
                    help="Target to scan")
parser.add_argument("--confidence", type=int, default=70,
                    help="Minimum confidence threshold (0-100, default 70)")
parser.add_argument("--output-dir", default=None,
                    help="Output directory (default: <repo>/research/<target>)")
args = parser.parse_args()

TARGET = args.target
CONFIDENCE_THRESHOLD = args.confidence

# BASE = repo root（脚本在 scripts/ 里，所以 parent.parent = repo root）
BASE = Path(__file__).resolve().parent.parent
EXTRACTS_MAP = {
    "imagemagick":  BASE / "scripts/extracts/imagemagick_712",
    "poppler":      BASE / "scripts/extracts/poppler",
    "linux_bpf":    BASE / "scripts/extracts/linux_bpf",
    "linux_kvm":    BASE / "scripts/extracts/linux_kvm",
    "linux_usb":    BASE / "scripts/extracts/linux_usb",
    "libxml2":      BASE / "scripts/extracts/libxml2",
    "libtiff":      BASE / "scripts/extracts/libtiff",
}

FOCUS_MAP = {
    "imagemagick":  "out-of-bounds read/write, heap buffer overflow, integer overflow in image decoding (TIFF/PNG/JPEG/JXL/HEIC coders)",
    "poppler":      "denial of service, infinite loop, integer overflow, heap OOB in PDF parsing (XRef, JBIG2, JPX streams)",
    "linux_bpf":    "container escape, eBPF verifier bypass, out-of-bounds map access, speculative execution, privilege escalation from unprivileged container",
    "linux_kvm":    "VM guest-to-host escape, KVM hypercall OOB, x86 instruction emulation bugs, memory corruption in hypervisor",
    "linux_usb":    "kernel crash on USB device connect, descriptor parsing OOB, NULL pointer dereference in probe(), use-after-free in driver binding",
    "libxml2":      "supply chain attack, XML/XPath parsing OOB, integer overflow, use-after-free, billion laughs variants",
    "libtiff":      "supply chain attack, TIFF parsing heap overflow, integer overflow in codec (PixarLog/LZW/ZIP)",
}

GLM_KEY = os.environ.get("GLM_API_KEY", "")
if not GLM_KEY:
    env_file = BASE / ".env"
    if env_file.exists():
        for line in env_file.read_text().splitlines():
            if line.startswith("GLM_API_KEY="):
                GLM_KEY = line.split("=", 1)[1].strip()

if not GLM_KEY:
    print("❌ GLM_API_KEY not found (set env var or add to .env)")
    sys.exit(1)

client = ZhipuAI(api_key=GLM_KEY) if HAS_ZHIPU else None

SYSTEM_PROMPT = """You are an elite security researcher performing vulnerability analysis on C/C++ code.
Your task: identify specific security vulnerabilities with high confidence.

For each finding, output EXACTLY this JSON format (one finding per line):
{"file":"<filename>","line":"<approx line>","type":"<vuln type>","severity":"<CRITICAL|HIGH|MEDIUM|LOW>","confidence":<0-100>,"description":"<technical description>","poc_hint":"<how to trigger>"}

Focus on EXPLOITABLE bugs only. Do NOT report:
- Style issues
- Already-guarded conditions  
- Theoretical issues without clear exploit path

If no vulnerabilities found, output: {"result":"clean"}"""

def scan_file(extract_path, focus):
    code = Path(extract_path).read_text(errors='replace')[:5000]
    
    user_msg = f"""Analyze this {TARGET} code for {focus}.

{code}

Report only HIGH confidence (>70%) findings in the JSON format specified."""

    if not client:
        return None
    
    try:
        resp = client.chat.completions.create(
            model="glm-4-flash",
            messages=[
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_msg}
            ],
            max_tokens=1000,
            temperature=0.1,
        )
        return resp.choices[0].message.content
    except Exception as e:
        return f"ERROR: {e}"


extracts_dir = EXTRACTS_MAP.get(TARGET)
if not extracts_dir or not extracts_dir.exists():
    print(f"❌ 未找到 extracts: {extracts_dir}")
    sys.exit(1)

focus = FOCUS_MAP.get(TARGET, "security vulnerabilities")
files = sorted(extracts_dir.glob("*.c"))
print(f"🔍 扫描目标: {TARGET} ({len(files)} 个 extracts)")
print(f"📌 关注点: {focus[:80]}...")

if args.output_dir:
    results_dir = Path(args.output_dir)
else:
    results_dir = BASE / f"research/{TARGET}"
results_dir.mkdir(parents=True, exist_ok=True)
out_file = results_dir / f"t1_glm_phase2_{datetime.now().strftime('%H%M')}.jsonl"

findings = []
errors = 0
clean = 0

for i, fpath in enumerate(files):
    print(f"  [{i+1:3d}/{len(files)}] {fpath.name[:50]}", end=" ", flush=True)
    
    result = scan_file(fpath, focus)
    
    if not result:
        print("(跳过)")
        continue
    
    if "ERROR" in result:
        errors += 1
        print(f"ERR")
        time.sleep(5)
        continue
    
    # 解析 JSON findings
    found_vuln = False
    for line in result.strip().splitlines():
        line = line.strip()
        if not line or not line.startswith('{'):
            continue
        try:
            obj = json.loads(line)
            if obj.get("result") == "clean":
                clean += 1
                continue
            if obj.get("confidence", 0) >= CONFIDENCE_THRESHOLD:
                obj["source_file"] = str(fpath.name)
                findings.append(obj)
                with open(out_file, 'a') as f:
                    f.write(json.dumps(obj) + '\n')
                found_vuln = True
        except:
            pass
    
    print("🚨" if found_vuln else "✓")
    time.sleep(20)  # rate limit

print(f"\n{'='*50}")
print(f"✅ 完成: {len(findings)} findings, {clean} clean, {errors} errors")
print(f"📄 结果: {out_file}")

# 汇总高置信度 findings
high = [f for f in findings if f.get('confidence', 0) >= 80]
if high:
    print(f"\n🔴 高置信度 findings ({len(high)}):")
    for f in high:
        print(f"  [{f['severity']}] {f['type']} @ {f['source_file']} (conf={f['confidence']})")
        print(f"    {f['description'][:100]}")
