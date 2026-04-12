"""Second-pass verification of vulnerability findings.

Stage 4: Use a (possibly different) model instance to independently
verify each finding, filtering false positives.

Reference: 红队博客 — "198 manually reviewed, 89% severity exactly matched"
"""

from __future__ import annotations

import logging

from cyberai.models.base import SecurityAgent, VulnFinding

logger = logging.getLogger(__name__)


async def verify_findings(
    findings: list[VulnFinding],
    agent: SecurityAgent,
    *,
    min_confidence: float = 0.3,
) -> list[VulnFinding]:
    """Verify a list of findings using a second model pass.

    Args:
        findings: Raw findings from Stage 2.
        agent: SecurityAgent to use for verification (can be different model).
        min_confidence: Minimum confidence to keep a finding.

    Returns:
        Filtered list of verified findings.
    """
    verified: list[VulnFinding] = []

    for finding in findings:
        try:
            assessed = await agent.assess_severity(finding)

            if assessed.confidence >= min_confidence:
                verified.append(assessed)
                logger.info(
                    "VERIFIED [%s] %s (confidence: %.2f)",
                    assessed.severity.value,
                    assessed.title,
                    assessed.confidence,
                )
            else:
                logger.info(
                    "REJECTED %s (confidence: %.2f < %.2f threshold)",
                    finding.title,
                    assessed.confidence,
                    min_confidence,
                )
        except Exception as e:
            logger.warning("Verification failed for %s: %s", finding.id, e)
            # Keep unverified findings with a note
            finding.confidence = -1.0  # mark as unverified
            verified.append(finding)

    return verified
