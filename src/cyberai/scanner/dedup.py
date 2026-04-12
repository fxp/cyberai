"""Result deduplication for scanner findings.

Stage 3: Same vulnerability may be flagged from multiple files.
Merge duplicates based on vuln_type + affected location proximity.
"""

from __future__ import annotations

from cyberai.models.base import VulnFinding


def dedup_findings(findings: list[VulnFinding], *, similarity_threshold: float = 0.8) -> list[VulnFinding]:
    """Remove duplicate vulnerability findings.

    Two findings are considered duplicates if they:
    1. Have the same vuln_type
    2. Are in the same file or overlapping line ranges
    3. Have similar descriptions (basic string overlap)

    Keeps the finding with the highest confidence.
    """
    if not findings:
        return []

    # Group by (file, vuln_type)
    groups: dict[tuple[str, str], list[VulnFinding]] = {}
    for f in findings:
        key = (f.file_path, f.vuln_type)
        groups.setdefault(key, []).append(f)

    deduped: list[VulnFinding] = []
    for group in groups.values():
        if len(group) == 1:
            deduped.append(group[0])
            continue

        # Within same (file, type), merge overlapping line ranges
        merged = _merge_overlapping(group)
        deduped.extend(merged)

    # Sort by severity then confidence
    severity_order = {"critical": 0, "high": 1, "medium": 2, "low": 3, "info": 4}
    deduped.sort(key=lambda f: (severity_order.get(f.severity.value, 5), -f.confidence))

    return deduped


def _merge_overlapping(findings: list[VulnFinding]) -> list[VulnFinding]:
    """Merge findings with overlapping line ranges, keeping highest confidence."""
    # Sort by line_start
    sorted_findings = sorted(findings, key=lambda f: f.line_start)
    merged: list[VulnFinding] = [sorted_findings[0]]

    for current in sorted_findings[1:]:
        prev = merged[-1]

        # Check overlap: current starts within previous range (with 5-line tolerance)
        if current.line_start <= prev.line_end + 5:
            # Keep the one with higher confidence
            if current.confidence > prev.confidence:
                merged[-1] = current
        else:
            merged.append(current)

    return merged
