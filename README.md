# CyberAI

> LLM-driven vulnerability research platform. Scans production C/C++ open-source
> libraries with GLM-5.1, cross-validates findings, and produces CVE-ready
> disclosure reports under coordinated 90-day disclosure.

**Status**: Active research · 2026
**Model**: glm-5.1 (BigModel) for detection + verification, glm-4-plus for cross-check
**Site**: https://fxp.github.io/cyberai/ · [docs/index.html](./docs/index.html)
**Architecture**: [docs/architecture.html](./docs/architecture.html)

## Pipeline

```
A. scan_<target>_t1.py       Pipeline A    glm-5.1   per-target static-extract
C. run_daily_scans.sh        Orchestrator  glm-5.1   ~20h, all 10 targets
B. .github/workflows/        Pipeline B    glm-5.1   agentic, GHA, any repo
   pipeline_b.yml
H. verify_findings.py        Adversarial   glm-5.1   re-judge CRITICAL+HIGH
J1. validate_findings.py     NVD CVE       (no LLM)  filter known-published
J2. validate_findings.py     Grounding     (no LLM)  drop unfindable extracts
J3. validate_findings.py     Cross-model   glm-4-plus second-opinion w/ code
J5. generate_drafts.py       Drafts        glm-5.1   coordinated-disclosure email
```

A full A→H→J1+J2+J3→J5 cycle on the ECS produces ~5-15 high-confidence
candidate findings per ~$20 spend.

## Targets scanned

| Target | Status |
|---|---|
| **libpng 1.6.45 — 1.6.58** | 🟢 **Primary lead** — `png_combine_row` integer overflow grounded; 32-bit ASAN PoC pending |
| libxml2 2.13.5 | 🟡 J3 partial — `xmlXPathNextAncestor` type confusion, exploitability narrow |
| ImageMagick 7.1.2 | ⚠ Pending verification (CAND-005, 006/007) |
| Eclipse Mosquitto 2.0.21 | ⚪ Reported 2026-04-18, deprioritized after expert review (low severity DoS) |
| libssh2 1.11.1 | ⚠ Many H-CONFIRMED but J3 grounding failed; needs better extracts |
| freetype 2.13.3 | ⚠ Same as libssh2 |
| expat 2.6.4 | ⚠ Same |
| sqlite 3.49.1 | ✓ Audited (mostly known-CVE recall) |
| openssl 3.4.1 | ✓ Audited |
| nginx 1.27.4 | ✓ Audited |
| zlib 1.3.1 | ✓ Clean |
| curl 8.11.0 | ✓ Clean |

The latest run summary is at
[`research/scan-2026-05-04/README.md`](./research/scan-2026-05-04/README.md).

## For agents (Claude Code, Cursor, Codex, etc.)

Read [`AGENTS.md`](./AGENTS.md) before doing anything in this repo. It
documents the ECS, OSS, GHA infrastructure, every script's purpose +
inputs/outputs, common operations, known pitfalls, cost reference, and
the disclosure protocol.

## Security & ethics

- All findings are reported privately to upstream maintainers before any
  public mention.
- Technical detail of unconfirmed candidates is kept confidential.
- Proof-of-concept code is not released until a patch is published.
- 90-day default disclosure window per Google Project Zero standards.

For research inquiries, open a GitHub issue. For security disclosures
about a specific finding, contact maintainers directly via the channel
documented in the relevant draft email.

## License

Defensive security research. Code under MIT (where applicable).
Vulnerability data and disclosure drafts are NOT for redistribution.
