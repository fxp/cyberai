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
            "Report a confirmed vulnerability. Only call this if you can trace the full "
            "exploit path — input source → vulnerable operation — with no blocking guards "
            "in the call chain. Do NOT report if any caller validates the input."
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
                    "description": "0-100. Only report if >= 80 and full exploit path traced."
                },
                "location": {
                    "type": "string",
                    "description": "file:function:line"
                },
                "description": {
                    "type": "string",
                    "description": "Precise technical description of the vulnerability"
                },
                "exploit_path": {
                    "type": "string",
                    "description": "Full call chain from external input to vulnerable operation"
                },
                "existing_guards": {
                    "type": "string",
                    "description": "List any guards you found and why they do NOT prevent exploitation"
                },
                "poc_hint": {
                    "type": "string",
                    "description": "Concrete PoC trigger (input format, value, call sequence)"
                }
            },
            "required": ["type", "severity", "confidence", "location",
                         "description", "exploit_path"]
        }
    }
]

SYSTEM_PROMPT = """You are a senior security researcher performing a deep code audit.

Your task: find real, exploitable vulnerabilities in the target file.

Rules:
1. You MUST use read_file and grep_repo to verify any claim before reporting.
   - If you see a suspicious pattern, READ the callers to check for input validation.
   - If you see an allocation, GREP for where the size parameter comes from.
   - If you see a bounds check is missing, CHECK if the caller validates bounds instead.
2. Only call report_finding if you have traced the FULL exploit path:
   external input → function → vulnerable operation, with NO blocking guards found.
3. Be skeptical. Assume guards exist. Prove they don't before reporting.
4. A finding where you cannot identify the input source is NOT reportable — mark it SKIP.
5. Cost of a false positive: wasted human review time. Cost of a false negative: missed bug.
   Prefer precision over recall.

You may make up to 15 tool calls per file. Use them wisely."""


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

    user_msg = f"""Audit this file for security vulnerabilities.

File: {file_path}

```c
{initial_code}
```

Use read_file and grep_repo to verify any suspicious pattern before reporting.
Remember: only report_finding when you have traced the full exploit path."""

    messages = [{"role": "user", "content": user_msg}]
    findings = []
    tool_calls = 0

    while tool_calls < 15:
        response = client.messages.create(
            model=model,
            max_tokens=4096,
            system=SYSTEM_PROMPT,
            tools=TOOLS,
            messages=messages,
        )

        # 收集 report_finding 调用
        tool_uses = [b for b in response.content if b.type == "tool_use"]
        for tu in tool_uses:
            if tu.name == "report_finding":
                finding = tu.input.copy()
                finding["source_file"] = file_path
                finding["model"] = model
                findings.append(finding)

        if response.stop_reason == "end_turn" or not tool_uses:
            break

        # 执行工具调用，返回结果
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

    return findings


def scan_file_glm(client, model: str, repo_root: Path, file_path: str) -> list[dict]:
    """Run agentic scan using ZhipuAI (GLM-5.1) tool calling."""
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
            f"Audit this file for security vulnerabilities.\n\nFile: {file_path}\n\n"
            f"```c\n{initial_code}\n```\n\n"
            "Use read_file and grep_repo to verify any suspicious pattern before reporting."
        )},
    ]

    findings = []
    tool_calls_count = 0

    while tool_calls_count < 15:
        resp = client.chat.completions.create(
            model=model,
            messages=messages,
            tools=glm_tools,
            tool_choice="auto",
            max_tokens=4096,
            temperature=0.05,
        )
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

            if tc.function.name == "report_finding":
                finding = inputs.copy()
                finding["source_file"] = file_path
                finding["model"] = model
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

    return findings


def main():
    parser = argparse.ArgumentParser(description="Pipeline B agentic detector")
    parser.add_argument("--target", required=True, help="Target repo path")
    parser.add_argument("--files", required=True, help="JSON array of file paths to scan")
    parser.add_argument("--model", default="claude-opus-4-6")
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
