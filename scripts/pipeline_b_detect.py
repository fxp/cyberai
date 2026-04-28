#!/usr/bin/env python3
"""
Pipeline B — Step 2: Agentic Detector
对单个文件做带"查阅权"的深度漏洞检测。

关键改进（对标 VIDOC 方法）：
  - 模型可调用 read_file / grep_repo 工具查看任意相关文件
  - 不再是孤立的 5000 字符片段，而是"带上下文查阅"的函数级审计
  - 使用 function calling / tool use 模式
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

# ── 工具：根据模型选择 client ─────────────────────────────────────
def get_client(model: str):
    if model.startswith("claude"):
        import anthropic
        return "anthropic", anthropic.Anthropic(api_key=os.environ["ANTHROPIC_API_KEY"])
    elif model.startswith("glm") or model.startswith("chatglm"):
        from zhipuai import ZhipuAI
        return "zhipuai", ZhipuAI(api_key=os.environ["GLM_API_KEY"])
    elif model.startswith("gpt"):
        from openai import OpenAI
        return "openai", OpenAI(api_key=os.environ["OPENAI_API_KEY"])
    else:
        raise ValueError(f"Unsupported model: {model}")


# ── 工具定义（tool use schema） ───────────────────────────────────
TOOLS = [
    {
        "name": "note_signal",
        "description": (
            "Record a weak security signal that alone may NOT be fully exploitable, "
            "but could combine with other signals across files to form a kill chain.\n\n"
            "Use for:\n"
            "- INFO_LEAK: any read of internal pointer/address/heap metadata exposed to attacker\n"
            "- PARTIAL_CORRUPTION: off-by-one, truncation, integer wrap that might be chained\n"
            "- TYPE_CONFUSION: value interpreted as wrong type if attacker controls union/cast\n"
            "- PRIVILEGE_TRANSITION: point where privilege level changes or can be influenced\n"
            "- TAINT_SOURCE: attacker-controlled value enters the codebase here\n"
            "- TAINT_SINK: dangerous operation (exec/write/free) that accepts tainted data\n"
            "- DOUBLE_FREE_RISK: pointer freed in one branch but potentially accessible elsewhere\n"
            "- RACE_WINDOW: TOCTOU gap where state can change between check and use\n\n"
            "Lower bar than report_finding — you only need to be 60% confident the signal "
            "exists and is reachable. The chain builder will correlate signals later."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "signal_type": {
                    "type": "string",
                    "enum": ["INFO_LEAK", "PARTIAL_CORRUPTION", "TYPE_CONFUSION",
                             "PRIVILEGE_TRANSITION", "TAINT_SOURCE", "TAINT_SINK",
                             "DOUBLE_FREE_RISK", "RACE_WINDOW"]
                },
                "location": {
                    "type": "string",
                    "description": "file:function:line"
                },
                "description": {
                    "type": "string",
                    "description": "What specifically happens at this point"
                },
                "data_involved": {
                    "type": "string",
                    "description": "What data is leaked/corrupted/tainted (type, size, content)"
                },
                "attacker_reach": {
                    "type": "string",
                    "description": "How an attacker can reach this code path (even if not fully traced)"
                },
                "chain_potential": {
                    "type": "string",
                    "description": "What signal type would combine with this to complete an exploit (e.g. 'INFO_LEAK here + PARTIAL_CORRUPTION in heap allocator = ASLR bypass + RCE')"
                },
                "confidence": {
                    "type": "integer",
                    "description": "0-100, minimum 60 to call this tool"
                }
            },
            "required": ["signal_type", "location", "description",
                         "attacker_reach", "chain_potential", "confidence"]
        }
    },
    {
        "name": "read_file",
        "description": (
            "Read the content of any source file in the target repository. "
            "Use this to inspect callers, callees, header definitions, "
            "or any adjacent code that might contain bounds checks or validation "
            "that is relevant to the file under review."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Relative path from repo root (e.g. 'src/net/socket.c')"
                },
                "start_line": {
                    "type": "integer",
                    "description": "First line to read (1-indexed, optional)"
                },
                "end_line": {
                    "type": "integer",
                    "description": "Last line to read (optional, default: start_line+150)"
                }
            },
            "required": ["path"]
        }
    },
    {
        "name": "grep_repo",
        "description": (
            "Search the repository for a function name, variable, macro, or pattern. "
            "Use to find where a function is defined, called, or where a struct is allocated."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "pattern": {
                    "type": "string",
                    "description": "grep-style regex pattern"
                },
                "path_filter": {
                    "type": "string",
                    "description": "Optional glob to restrict search (e.g. '*.c', 'net/**')"
                }
            },
            "required": ["pattern"]
        }
    },
    {
        "name": "report_finding",
        "description": (
            "Report a CONFIRMED vulnerability. Call ONLY when ALL of the following are true:\n"
            "1. You have read the actual vulnerable code with read_file (not guessed).\n"
            "2. You have traced the COMPLETE path: external_input → call chain → vulnerable op.\n"
            "3. You have grepped for guards/validators in ALL callers and found NONE.\n"
            "4. The input is reachable from an unprivileged network/file/user source.\n"
            "5. confidence >= 85.\n\n"
            "DO NOT call for:\n"
            "- strdup(s), strndup(s,n): these allocate dynamically, no fixed buffer.\n"
            "- malloc(strlen(s)+1): dynamic allocation, cannot overflow.\n"
            "- snprintf(buf, sizeof(buf), ...): bounded by sizeof, safe by definition.\n"
            "- calloc(n, sz): overflow-safe in POSIX libc (returns NULL on overflow).\n"
            "- Any function in test/, tests/, examples/, doc/ directories.\n"
            "- Anything where you found a guard but 'aren't sure if it's sufficient'.\n"
            "- Patterns that look dangerous but where you haven't read the calling code."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "type": {
                    "type": "string",
                    "description": "Vulnerability class (e.g. 'Buffer Overflow', 'Integer Overflow', 'Use-After-Free')"
                },
                "severity": {
                    "type": "string",
                    "enum": ["CRITICAL", "HIGH", "MEDIUM", "LOW"]
                },
                "confidence": {
                    "type": "integer",
                    "description": "0-100. Must be >= 85 to call this tool."
                },
                "location": {
                    "type": "string",
                    "description": "file:function:line"
                },
                "description": {
                    "type": "string",
                    "description": "Precise technical description of the vulnerability"
                },
                "input_source": {
                    "type": "string",
                    "description": "Where the attacker-controlled input enters (e.g. 'network packet field', 'XML attribute value', 'filename argument')"
                },
                "exploit_path": {
                    "type": "string",
                    "description": "Full call chain: func_a() -> func_b(tainted) -> vulnerable_op. Must include actual function names and line numbers."
                },
                "guards_checked": {
                    "type": "string",
                    "description": "REQUIRED: List every validation/guard you checked in callers, and explain specifically why each one does NOT prevent exploitation."
                },
                "poc_hint": {
                    "type": "string",
                    "description": "Concrete PoC trigger (input format, value, call sequence)"
                }
            },
            "required": ["type", "severity", "confidence", "location",
                         "description", "input_source", "exploit_path", "guards_checked"]
        }
    }
]

SYSTEM_PROMPT = """You are an expert offensive security researcher building a kill chain map of a codebase.

## Primary goal: map ALL weak signals, not just standalone bugs

Real-world exploits are rarely a single bug. They are chains:
  INFO_LEAK → ASLR bypass → PARTIAL_CORRUPTION → arbitrary write → RCE
  TAINT_SOURCE → TAINT_SINK → command injection
  RACE_WINDOW + DOUBLE_FREE_RISK → UAF → controlled allocation → code exec

Your job is TWO things:
  1. note_signal for EVERY interesting weak point (low bar: confidence >= 60)
  2. report_finding ONLY for standalone fully-confirmed bugs (high bar: confidence >= 85)

## What to note_signal

Think like an attacker building a chain:
- Anywhere attacker-controlled data enters without full sanitization → TAINT_SOURCE
- Anywhere a dangerous op (exec, write-to-fd, free, memcpy-to-stack) takes externally-derived size or pointer → TAINT_SINK
- Anywhere an internal address, heap pointer, or secret is accessible to an attacker → INFO_LEAK
- Any integer that could wrap under attacker control → PARTIAL_CORRUPTION
- Any union/type cast where attacker controls the active member → TYPE_CONFUSION
- Any TOCTOU gap (check then use with a window in between) → RACE_WINDOW
- Any pointer freed in one branch with a code path that accesses it in another → DOUBLE_FREE_RISK

## Investigation workflow

1. read_file: see the code for real before noting anything.
2. grep_repo: find callers, definitions, where data comes from.
3. note_signal: record every interesting point with chain_potential filled in.
4. If you find a case where two signals you noted IN THIS FILE already form a complete chain: call report_finding.

## Known-safe (do NOT even note_signal)

- `strdup(s)` / `malloc(strlen(s)+1)`: dynamic, no overflow.
- `snprintf(buf, sizeof(buf), ...)`: bounded.
- `calloc(n, sz)`: overflow-safe by POSIX.
- Files in test/, tests/, examples/, fuzz/, docs/.

You have 25 tool calls. Use them to build a complete signal map, not to find one perfect bug."""


def execute_tool(name: str, inputs: dict, repo_root: Path) -> str:
    """Execute a tool call and return the result as a string."""
    if name == "read_file":
        path = repo_root / inputs["path"]
        if not path.exists():
            return f"ERROR: file not found: {inputs['path']}"
        try:
            lines = path.read_text(errors="replace").splitlines()
        except Exception as e:
            return f"ERROR reading file: {e}"
        start = max(0, inputs.get("start_line", 1) - 1)
        end = inputs.get("end_line", start + 150)
        chunk = lines[start:end]
        result = f"// {inputs['path']} (lines {start+1}-{start+len(chunk)})\n"
        result += "\n".join(f"{start+i+1:4d}  {l}" for i, l in enumerate(chunk))
        return result[:6000]  # 上限 6000 字符

    elif name == "grep_repo":
        pattern = inputs["pattern"]
        path_filter = inputs.get("path_filter", "")
        cmd = ["grep", "-rn", "--include=*.c", "--include=*.cc",
               "--include=*.cpp", "--include=*.h",
               "-m", "20", pattern]
        if path_filter:
            cmd = ["grep", "-rn", f"--include={path_filter}",
                   "-m", "20", pattern]
        try:
            result = subprocess.run(
                cmd, cwd=repo_root, capture_output=True, text=True, timeout=10
            )
            out = result.stdout.strip()
            if not out:
                return f"No matches for: {pattern}"
            return out[:3000]
        except Exception as e:
            return f"ERROR: {e}"

    elif name == "note_signal":
        return "SIGNAL_RECORDED"

    elif name == "report_finding":
        return "FINDING_RECORDED"

    return f"ERROR: unknown tool {name}"


def scan_file_anthropic(client, model: str, repo_root: Path, file_path: str) -> list[dict]:
    """Run agentic scan on one file using Anthropic tool use."""
    import anthropic
    full_path = repo_root / file_path
    try:
        content = full_path.read_text(errors="replace")
    except Exception as e:
        return []

    # 截取前 8000 字符作为初始上下文（模型可以用工具查更多）
    initial_code = content[:8000]
    if len(content) > 8000:
        initial_code += f"\n\n// ... [{len(content)-8000} more bytes — use read_file to see more] ..."

    user_msg = f"""Map all security signals in this file for kill chain analysis.

File: {file_path}

```c
{initial_code}
```

Use note_signal for every interesting weak point (confidence >= 60).
Use read_file/grep_repo to verify before noting.
Use report_finding only for standalone confirmed bugs (confidence >= 85)."""

    messages = [{"role": "user", "content": user_msg}]
    findings = []   # confirmed standalone bugs
    signals = []    # weak signals for chain analysis
    tool_calls = 0

    while tool_calls < 25:
        response = client.messages.create(
            model=model,
            max_tokens=4096,
            system=SYSTEM_PROMPT,
            tools=TOOLS,
            messages=messages,
        )

        tool_uses = [b for b in response.content if b.type == "tool_use"]
        for tu in tool_uses:
            if tu.name == "note_signal":
                sig = tu.input.copy()
                if sig.get("confidence", 0) >= 60:
                    sig["source_file"] = file_path
                    sig["model"] = model
                    sig["_record_type"] = "signal"
                    signals.append(sig)
            elif tu.name == "report_finding":
                finding = tu.input.copy()
                if finding.get("confidence", 0) >= 85:
                    finding["source_file"] = file_path
                    finding["model"] = model
                    finding["_record_type"] = "finding"
                    findings.append(finding)

        if response.stop_reason == "end_turn" or not tool_uses:
            break

        tool_results = []
        for tu in tool_uses:
            tool_calls += 1
            result = execute_tool(tu.name, tu.input, repo_root)
            tool_results.append({
                "type": "tool_result",
                "tool_use_id": tu.id,
                "content": result,
            })

        messages.append({"role": "assistant", "content": response.content})
        messages.append({"role": "user", "content": tool_results})

    return findings + signals  # 合并输出，chain builder 区分 _record_type


def glm_chat_with_retry(client, model, messages, tools, max_retries=5):
    """ZhipuAI chat with exponential backoff on 429 rate-limit errors."""
    import time
    delay = 10
    for attempt in range(max_retries):
        try:
            return client.chat.completions.create(
                model=model,
                messages=messages,
                tools=tools,
                tool_choice="auto",
                max_tokens=4096,
                temperature=0.05,
            )
        except Exception as e:
            err = str(e)
            if "429" in err or "1313" in err or "rate" in err.lower():
                if attempt < max_retries - 1:
                    wait = delay * (2 ** attempt)
                    print(f"    ⏳ 429 rate limit, retrying in {wait}s (attempt {attempt+1}/{max_retries})")
                    time.sleep(wait)
                else:
                    raise
            else:
                raise
    raise RuntimeError("GLM retry exhausted")


def scan_file_glm(client, model: str, repo_root: Path, file_path: str) -> list[dict]:
    """Run agentic scan using ZhipuAI tool calling."""
    full_path = repo_root / file_path
    try:
        content = full_path.read_text(errors="replace")
    except Exception:
        return []

    initial_code = content[:8000]
    if len(content) > 8000:
        initial_code += f"\n// ... [{len(content)-8000} more bytes — use read_file to see more] ..."

    # ZhipuAI tool schema（OpenAI 风格）
    glm_tools = [
        {
            "type": "function",
            "function": {
                "name": t["name"],
                "description": t["description"],
                "parameters": t["input_schema"],
            }
        }
        for t in TOOLS
    ]

    messages = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user", "content": (
            f"Map all security signals in this file for kill chain analysis.\n\n"
            f"File: {file_path}\n\n```c\n{initial_code}\n```\n\n"
            "Use note_signal for every weak point (confidence >= 60).\n"
            "Use read_file/grep_repo to verify. report_finding only for confirmed bugs (>= 85)."
        )},
    ]

    findings = []
    signals = []
    tool_calls_count = 0

    while tool_calls_count < 25:
        resp = glm_chat_with_retry(client, model, messages, glm_tools)
        msg = resp.choices[0].message
        messages.append(msg.model_dump())

        if not msg.tool_calls:
            break

        tool_results = []
        for tc in msg.tool_calls:
            tool_calls_count += 1
            try:
                inputs = json.loads(tc.function.arguments)
            except Exception:
                inputs = {}

            if tc.function.name == "note_signal":
                if inputs.get("confidence", 0) >= 60:
                    sig = inputs.copy()
                    sig["source_file"] = file_path
                    sig["model"] = model
                    sig["_record_type"] = "signal"
                    signals.append(sig)
                result = "SIGNAL_RECORDED"
            elif tc.function.name == "report_finding":
                if inputs.get("confidence", 0) >= 85:
                    finding = inputs.copy()
                    finding["source_file"] = file_path
                    finding["model"] = model
                    finding["_record_type"] = "finding"
                    findings.append(finding)
                result = "FINDING_RECORDED"
            else:
                result = execute_tool(tc.function.name, inputs, repo_root)

            tool_results.append({
                "role": "tool",
                "tool_call_id": tc.id,
                "content": result,
            })

        messages.extend(tool_results)

    return findings + signals


def main():
    parser = argparse.ArgumentParser(description="Pipeline B agentic detector")
    parser.add_argument("--target", required=True, help="Target repo path")
    parser.add_argument("--files", required=True, help="JSON array of file paths to scan")
    parser.add_argument("--model", default="glm-4-flash")
    parser.add_argument("--output", required=True, help="Output JSONL path")
    args = parser.parse_args()

    repo_root = Path(args.target).resolve()

    try:
        parsed = json.loads(args.files)
        # workflow 传入的是对象数组 [{"path":..., "priority":...}, ...]
        # 也接受纯字符串数组 ["a.c", "b.c"]
        if parsed and isinstance(parsed[0], dict):
            files = [item["path"] for item in parsed if "path" in item]
        else:
            files = [str(f) for f in parsed]
    except Exception:
        # 降级：换行或逗号分隔的路径列表
        raw = args.files.replace(",", "\n")
        files = [f.strip() for f in raw.splitlines() if f.strip()]

    provider, client = get_client(args.model)
    print(f"🤖 模型: {args.model} ({provider})")
    print(f"📁 目标: {repo_root}")
    print(f"📄 文件: {len(files)} 个")

    all_findings = []
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, "w") as out:
        for i, file_path in enumerate(files):
            print(f"  [{i+1}/{len(files)}] {file_path}", end=" ", flush=True)
            start = time.time()

            try:
                if provider == "anthropic":
                    findings = scan_file_anthropic(client, args.model, repo_root, file_path)
                else:
                    findings = scan_file_glm(client, args.model, repo_root, file_path)
            except Exception as e:
                print(f"ERROR: {e}")
                findings = []

            elapsed = time.time() - start
            print(f"→ {len(findings)} findings ({elapsed:.1f}s)")

            for f in findings:
                out.write(json.dumps(f, ensure_ascii=False) + "\n")
            all_findings.extend(findings)

            time.sleep(1)  # 避免 API 限速

    print(f"\n✅ 完成: {len(all_findings)} findings → {output_path}")


if __name__ == "__main__":
    main()
