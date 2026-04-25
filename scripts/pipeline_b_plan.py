#!/usr/bin/env python3
"""
Pipeline B — Step 1: Scan Planner
对目标仓库的 C/C++ 文件按安全敏感度排序，生成扫描计划。
输出 JSON: { "target": ..., "files": [{"path": ..., "priority": ..., "reason": ...}] }
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

# ── 安全敏感文件启发式规则 ────────────────────────────────────────
# 优先级分：100=最高，0=跳过
PRIORITY_RULES = [
    # 高价值：认证 / 加密 / 协议解析
    (100, ["auth", "crypto", "tls", "ssl", "x509", "cert", "token", "session",
           "login", "oauth", "jwt", "hmac", "sign", "verify", "trust"]),
    # 高价值：内存管理 / 分配
    (90,  ["alloc", "malloc", "realloc", "heap", "pool", "arena", "buffer",
           "mem", "gc", "slab"]),
    # 高价值：解析器 / 反序列化
    (85,  ["parse", "parser", "decode", "deserializ", "unpack", "read",
           "lexer", "scanner", "tokenize", "inflate", "decompress"]),
    # 高价值：网络 / RPC
    (80,  ["socket", "recv", "send", "packet", "network", "rpc", "http",
           "proto", "nfs", "smb", "ssh", "mqtt", "amqp"]),
    # 高价值：内核 / 特权路径
    (75,  ["syscall", "ioctl", "kvm", "bpf", "verif", "ebpf", "irq",
           "interrupt", "privilege", "suid", "setuid", "capability"]),
    # 中等：文件 I/O / 路径处理
    (60,  ["file", "path", "dir", "open", "close", "read", "write",
           "seek", "mmap", "fopen", "fread"]),
    # 低价值：测试 / 工具
    (10,  ["test", "bench", "example", "demo", "sample", "tool", "util"]),
    # 跳过：文档 / 生成文件
    (0,   ["doc", "docs", "generated", "autogen", "third_party", "vendor",
           "compat", "compat_"]),
]

SKIP_EXTENSIONS = {".h", ".hpp", ".md", ".txt", ".rst", ".cmake",
                   ".mk", ".am", ".ac", ".m4", ".py", ".sh"}
TARGET_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx"}


def score_file(path: Path, repo_root: Path) -> tuple[int, str]:
    """返回 (priority_score, reason)"""
    rel = str(path.relative_to(repo_root)).lower()
    name = path.stem.lower()

    for score, keywords in PRIORITY_RULES:
        for kw in keywords:
            if kw in rel or kw in name:
                if score == 0:
                    return 0, f"skip: matches '{kw}'"
                return score, f"matches '{kw}'"

    return 50, "default"  # 默认中等优先级


def find_c_files(repo_root: Path, paths_filter: list[str]) -> list[Path]:
    """找出所有 C/C++ 文件"""
    files = []
    search_roots = []

    if paths_filter:
        for p in paths_filter:
            full = repo_root / p.strip()
            if full.exists():
                search_roots.append(full)
    else:
        search_roots = [repo_root]

    for root in search_roots:
        if root.is_file() and root.suffix in TARGET_EXTENSIONS:
            files.append(root)
        else:
            for ext in TARGET_EXTENSIONS:
                files.extend(root.rglob(f"*{ext}"))

    # 过滤跳过扩展名（应该不需要，但保险起见）
    files = [f for f in files if f.suffix not in SKIP_EXTENSIONS]
    return files


def count_lines(path: Path) -> int:
    try:
        with open(path, "rb") as f:
            return f.read().count(b"\n")
    except Exception:
        return 0


def get_git_blame_score(path: Path, repo_root: Path) -> int:
    """文件最近修改次数（越多=越活跃=优先级加分）。
    在 shallow clone（GitHub Actions --depth=1）中 log 输出为空，返回 0 即可。
    """
    try:
        result = subprocess.run(
            ["git", "log", "--oneline", "-20", "--", str(path.relative_to(repo_root))],
            cwd=repo_root, capture_output=True, text=True, timeout=5
        )
        if result.returncode != 0:
            return 0
        return min(len(result.stdout.strip().splitlines()), 10)
    except Exception:
        return 0


def main():
    parser = argparse.ArgumentParser(description="Pipeline B scan planner")
    parser.add_argument("--target", required=True, help="Target repo path")
    parser.add_argument("--max-files", type=int, default=50)
    parser.add_argument("--paths", default="", help="Comma-separated sub-paths to scan")
    parser.add_argument("--model", default="glm-4-flash")
    parser.add_argument("--output", default="/tmp/scan_plan.json")
    args = parser.parse_args()

    repo_root = Path(args.target).resolve()
    paths_filter = [p for p in args.paths.split(",") if p.strip()] if args.paths else []

    print(f"🔍 扫描目标: {repo_root}")
    print(f"   max_files={args.max_files}, model={args.model}")

    # 1. 找所有 C/C++ 文件
    all_files = find_c_files(repo_root, paths_filter)
    print(f"   发现 {len(all_files)} 个 C/C++ 文件")

    # 2. 评分
    scored = []
    for f in all_files:
        score, reason = score_file(f, repo_root)
        if score == 0:
            continue
        lines = count_lines(f)
        if lines < 20:  # 太短的文件跳过
            continue
        # git 活跃度加分
        git_bonus = get_git_blame_score(f, repo_root)
        final_score = score + git_bonus
        scored.append({
            "path": str(f.relative_to(repo_root)),
            "abs_path": str(f),
            "priority": final_score,
            "reason": reason,
            "lines": lines,
        })

    # 3. 排序，截断
    scored.sort(key=lambda x: (-x["priority"], -x["lines"]))
    selected = scored[:args.max_files]

    plan = {
        "target": str(repo_root),
        "model": args.model,
        "total_found": len(all_files),
        "total_selected": len(selected),
        "files": selected,
    }

    with open(args.output, "w") as f:
        json.dump(plan, f, indent=2, ensure_ascii=False)

    print(f"\n📋 扫描计划: {len(selected)} 个文件（共 {len(all_files)} 个）")
    print(f"   Top 10 优先级文件:")
    for item in selected[:10]:
        print(f"   [{item['priority']:3d}] {item['path']}  ({item['lines']} lines, {item['reason']})")
    print(f"\n✅ 计划已写入: {args.output}")


if __name__ == "__main__":
    main()
