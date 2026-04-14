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

# ---------------------------------------------------------------------------
# Project-specific boosters
# Keyed by a substring that must appear anywhere in the project root path.
# Value is a dict of path-substring → bonus score.
# ---------------------------------------------------------------------------

PROJECT_BOOSTERS: dict[str, dict[str, float]] = {
    # ImageMagick: coders/ directory is the highest-CVE attack surface.
    # Specific format codecs are ordered by historical CVE density.
    "imagemagick": {
        "coders/png":    60,   # ~80 historical CVEs
        "coders/tiff":   55,
        "coders/psd":    50,
        "coders/svg":    50,   # ImageTragick origin
        "coders/gif":    48,
        "coders/pdf":    45,
        "coders/jp2":    44,
        "coders/bmp":    43,
        "coders/webp":   40,
        "coders/heic":   40,
        "coders/mvg":    38,   # ImageTragick core
        "coders/":       25,   # any other coder
        "magickcore/pixel":    20,
        "magickcore/memory":   20,
        "magickcore/constitute": 18,
        "magickcore/transform": 15,
        "magickcore/resize":   15,
    },
    # LibTIFF: key parsing and codec files
    "libtiff": {
        "tif_dirread":   55,
        "tif_read":      50,
        "tif_pixarlog":  45,
        "tif_luv":       44,
        "tif_getimage":  43,
        "tif_dirwrite":  40,
        "tif_compress":  38,
    },
    # Mosquitto MQTT broker
    "mosquitto": {
        "read_handle":   55,
        "packet_mosq":   50,
        "mqtt_protocol": 50,
        "handle_":       45,
        "net_mosq":      40,
        "src/":          20,
    },
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

    return files


def apply_project_boosters(
    files: list[RankedFile],
    root_dir: str | Path,
) -> list[RankedFile]:
    """Apply project-specific score bonuses based on known high-CVE paths.

    Checks the root directory name against PROJECT_BOOSTERS and adds bonus
    scores for files whose relative paths match known high-risk patterns.
    """
    root_lower = Path(root_dir).name.lower()

    # Find matching project booster (substring match on root dir name)
    booster: dict[str, float] = {}
    for project_key, path_map in PROJECT_BOOSTERS.items():
        if project_key in root_lower:
            booster = path_map
            break

    if not booster:
        return files

    for f in files:
        rel_lower = f.relative_path.lower()
        best_bonus = 0.0
        for path_substr, bonus in booster.items():
            if path_substr in rel_lower:
                best_bonus = max(best_bonus, bonus)
        f.score += best_bonus

    files.sort(key=lambda f: f.score, reverse=True)
    return files


def get_scan_queue(
    root_dir: str | Path,
    *,
    max_files: int | None = None,
    min_score: float = 0.0,
) -> list[RankedFile]:
    """Full Stage 1 pipeline: discover → rank → project-boost → filter → queue."""
    files = discover_files(root_dir)
    ranked = rank_files(files)
    ranked = apply_project_boosters(ranked, root_dir)

    # Filter by minimum score
    if min_score > 0:
        ranked = [f for f in ranked if f.score >= min_score]

    # Limit count
    if max_files is not None:
        ranked = ranked[:max_files]

    return ranked
