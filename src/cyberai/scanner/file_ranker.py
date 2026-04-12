"""File ranking and filtering for vulnerability scanning.

Stage 1 of the Carlini pipeline:
- Filter files by type (only security-relevant source files)
- Exclude tests, docs, configs
- Rank by security sensitivity (network > memory > file I/O > other)
"""

from __future__ import annotations

import os
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# File extensions to scan, ordered by typical security relevance
SCANNABLE_EXTENSIONS = {
    ".c", ".h",        # C — most kernel/system code
    ".cc", ".cpp", ".cxx", ".hh", ".hpp",  # C++
    ".rs",             # Rust (unsafe blocks)
    ".go",             # Go
    ".py",             # Python
    ".java",           # Java
    ".js", ".ts",      # JavaScript/TypeScript
    ".rb",             # Ruby
    ".php",            # PHP
    ".swift",          # Swift
    ".m", ".mm",       # Objective-C
}

# Directories to skip entirely
SKIP_DIRS = {
    ".git", ".svn", ".hg",
    "node_modules", "vendor", "third_party", "3rdparty",
    "__pycache__", ".tox", ".venv", "venv", "env",
    "build", "dist", "target", "out", "bin", "obj",
    "docs", "doc", "documentation",
    ".idea", ".vscode", ".settings",
}

# File name patterns indicating test files
TEST_PATTERNS = {
    "test_", "_test.", ".test.", "tests.", "spec.", "_spec.",
    "mock_", "_mock.", "fixture", "conftest",
}

# Keywords that indicate security-sensitive code (higher priority)
SECURITY_KEYWORDS = {
    # Network / protocol handling
    "socket", "recv", "send", "accept", "listen", "bind", "connect",
    "http", "rpc", "nfs", "smb", "dns", "tcp", "udp", "tls", "ssl",
    "packet", "protocol", "serialize", "deserialize", "parse",
    # Memory operations
    "malloc", "calloc", "realloc", "free", "memcpy", "memmove", "strcpy",
    "strncpy", "sprintf", "snprintf", "gets", "scanf",
    "alloc", "dealloc", "buffer", "heap", "stack",
    "unsafe", "raw_pointer", "transmute",  # Rust
    # File / IO
    "open", "read", "write", "fopen", "fread", "fwrite",
    "path", "filename", "directory",
    # Auth / crypto
    "password", "credential", "token", "secret", "key",
    "encrypt", "decrypt", "hash", "sign", "verify",
    "auth", "login", "session", "cookie", "jwt",
    # Privilege
    "privilege", "permission", "capability", "setuid", "root",
    "admin", "sudo", "exec", "system", "popen", "eval",
    # Kernel
    "ioctl", "syscall", "mmap", "ptrace", "kmod",
}


@dataclass
class RankedFile:
    """A file with its computed security-relevance score."""
    path: str
    relative_path: str
    extension: str
    size_bytes: int
    score: float = 0.0
    keywords_found: list[str] = field(default_factory=list)


def discover_files(root_dir: str | Path) -> list[RankedFile]:
    """Walk a directory tree and collect scannable source files."""
    root = Path(root_dir).resolve()
    files: list[RankedFile] = []

    for dirpath, dirnames, filenames in os.walk(root):
        # Prune skip dirs (modifying dirnames in-place)
        dirnames[:] = [d for d in dirnames if d.lower() not in SKIP_DIRS]

        for fname in filenames:
            ext = Path(fname).suffix.lower()
            if ext not in SCANNABLE_EXTENSIONS:
                continue

            # Skip test files
            fname_lower = fname.lower()
            if any(p in fname_lower for p in TEST_PATTERNS):
                continue

            full_path = Path(dirpath) / fname
            rel_path = full_path.relative_to(root)

            try:
                size = full_path.stat().st_size
            except OSError:
                continue

            # Skip very large files (>500KB) and empty files
            if size == 0 or size > 500_000:
                continue

            files.append(RankedFile(
                path=str(full_path),
                relative_path=str(rel_path),
                extension=ext,
                size_bytes=size,
            ))

    return files


def rank_files(files: list[RankedFile], *, sample_bytes: int = 4096) -> list[RankedFile]:
    """Score and sort files by security relevance.

    Reads the first `sample_bytes` of each file to look for security keywords.
    Higher score = more likely to contain vulnerabilities = scan first.
    """
    for f in files:
        score = 0.0

        # Extension weight: C/C++ > Rust > Go > scripting languages
        ext_weights = {
            ".c": 10, ".h": 9,
            ".cc": 9, ".cpp": 9, ".cxx": 9, ".hh": 8, ".hpp": 8,
            ".rs": 7, ".go": 6,
            ".py": 4, ".java": 4,
            ".js": 3, ".ts": 3,
            ".php": 5, ".rb": 3,
            ".swift": 4, ".m": 5, ".mm": 5,
        }
        score += ext_weights.get(f.extension, 1)

        # Keyword analysis (read file header)
        try:
            with open(f.path, "r", errors="ignore") as fh:
                sample = fh.read(sample_bytes).lower()

            found_keywords = []
            for kw in SECURITY_KEYWORDS:
                if kw in sample:
                    found_keywords.append(kw)
                    score += 2  # each keyword adds 2 points

            f.keywords_found = found_keywords
        except (OSError, UnicodeDecodeError):
            pass

        # Path-based hints
        rel_lower = f.relative_path.lower()
        if any(d in rel_lower for d in ("net/", "network/", "protocol/", "rpc/")):
            score += 15
        if any(d in rel_lower for d in ("crypto/", "auth/", "security/")):
            score += 12
        if any(d in rel_lower for d in ("kernel/", "driver/", "fs/")):
            score += 10
        if any(d in rel_lower for d in ("parser/", "codec/", "decode/")):
            score += 8

        f.score = score

    # Sort descending by score
    files.sort(key=lambda f: f.score, reverse=True)
    return files


def get_scan_queue(
    root_dir: str | Path,
    *,
    max_files: int | None = None,
    min_score: float = 0.0,
) -> list[RankedFile]:
    """Full Stage 1 pipeline: discover → rank → filter → return queue."""
    files = discover_files(root_dir)
    ranked = rank_files(files)

    # Filter by minimum score
    if min_score > 0:
        ranked = [f for f in ranked if f.score >= min_score]

    # Limit count
    if max_files is not None:
        ranked = ranked[:max_files]

    return ranked
