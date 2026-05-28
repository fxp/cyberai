# CyberAI — LLM-Driven Vulnerability Research Platform

> **Status**: Active Research | First disclosed vulnerability: CAND-008 (Mosquitto)  
> **Team**: Xiaoping Feng et al. | **Model**: GLM-5.1

---

## What is CyberAI?

CyberAI is a research project demonstrating that large language models can **discover real, previously unknown software vulnerabilities** — not just answer security questions.

We scan production C/C++ open-source libraries with GLM-5.1, cross-validate findings manually, and produce CVE-ready disclosure reports. Our first confirmed candidate, CAND-008 in Eclipse Mosquitto, is currently in responsible disclosure.

---

## Architecture: Pipeline A

```
Target Source Code (.c / .cpp)
        │
        ▼ Function-level slicing (Python)
[N function chunks]
        │
        ▼ GLM-5.1 sequential analysis
          Prompt: "Find buffer overflows, integer overflows, logic errors"
[Raw findings (~97% false positive rate)]
        │
        ▼ Automated triage: CVE DB cross-reference + confidence filter (≥80%)
[Candidate findings]
        │
        ▼ Manual + LLM cross-validation
[Confirmed vulnerabilities]
        │
        ▼ PoC + Disclosure draft
```

**Key insight**: The hard problem is not "getting LLM to find bugs" — it's filtering the 97% false positives to surface the 3% real ones.

---

## Targets & Status

| Target | Version | Segments Scanned | Candidates | Confirmed | Status |
|--------|---------|-----------------|-----------|-----------|--------|
| ImageMagick | 7.1.1-44 | 41/41 | 3 | 2 (GIF/BMP) | ⚠️ Pending verification |
| libpng | 1.6.43 | Full | 1 | 1 (candidate) | ⚠️ In review |
| curl | 8.11.0 | 14/14 | 0 | 0 | ✅ Clean |
| expat | 2.6.x | Full | 1 | Pending | ⚠️ In review |
| freetype | 2.13.x | Full | 1 | Pending | ⚠️ In review |
| **Eclipse Mosquitto** | **2.0.21** | **22/22** | **4** | **1 (CAND-008)** | **🔴 Disclosed** |
| LibTIFF | 4.7.0 | 39/43 | 2 | 0 (FP confirmed) | ✅ Audited |

---

## CAND-008: Eclipse Mosquitto — Our First Disclosure

- **Type**: Logic/resource management vulnerability in MQTT session handling
- **Component**: `src/session_expiry.c` / `src/handle_*.c`
- **Impact**: Potential denial-of-service or resource exhaustion in IoT broker deployments
- **Disclosure Timeline**:
  - 2026-04-18: Initial report sent to security@mosquitto.org
  - 2026-04-25: Follow-up if no response
  - 2026-07-17: Public disclosure (90-day deadline)
- **Status**: Awaiting vendor acknowledgment

---

## Repository Structure

```
cyberai/
├── src/                    # Scanner pipeline code
│   ├── scanner.py          # Core GLM-5.1 scanning engine
│   ├── slicer.py           # Function-level code slicing
│   └── triage.py           # Automated false-positive filtering
├── scripts/                # Utility scripts
├── targets/                # Per-target scan configs
├── research/
│   ├── triage_report_2026-04-18.md    # Full triage report
│   ├── technical_report_v1.md         # Research methodology
│   └── disclosures/                   # Disclosure drafts (redacted)
├── tests/                  # Validation tests
└── docker-compose.yml      # Self-contained scan environment
```

---

## Research Goals

1. **Prove LLMs can find real CVEs** — not just classify known vulnerabilities
2. **Quantify model capability** — benchmark GLM-5.1 vs Claude vs GPT-4o on security tasks
3. **Build reusable tooling** — open-source the scanner pipeline post-disclosure

---

## Security & Ethics

- We follow **Coordinated Vulnerability Disclosure (CVD)** per Google Project Zero standards
- All findings are privately reported to maintainers before any publication
- Technical details of unconfirmed candidates are kept confidential
- We do **not** release PoC code until patches are published

---

## Evaluation Framework (Upcoming)

After completing our first disclosure cycle, we plan to publish a comparative study:

| Model | Targets Scanned | True Positives | False Positive Rate | Time per 1k LOC |
|-------|----------------|---------------|-------------------|----------------|
| GLM-5.1 | 7 | TBD | ~97% | TBD |
| Claude 3.7 | TBD | TBD | TBD | TBD |
| GPT-4o | TBD | TBD | TBD | TBD |

---

## Running the Scanner

```bash
# Setup
git clone <repo>
cd cyberai
cp .env.example .env  # Add your GLM API key

# Run scan on a target
docker-compose up -d
python src/scanner.py --target mosquitto --version 2.0.21 --model glm-5.1

# View triage report
python src/triage.py --report research/scan_latest.json --confidence 0.8
```

**Requirements**: Docker, Python 3.11+, GLM-5.1 API access

---

## Contact

Research inquiries: [Create an issue] | Security disclosures follow CVD — contact project maintainers first.

---

*This project is for defensive security research only. All vulnerability details are handled per responsible disclosure protocols.*
