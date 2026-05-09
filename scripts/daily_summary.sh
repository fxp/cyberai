#!/usr/bin/env bash
# daily_summary.sh — fire after run_daily_scans.sh completes
#
# Aggregates the latest per-target results, has GLM-5.1 author a short
# Slack body (~80 words), and triggers .github/workflows/notify_slack.yml
# via gh CLI.
#
# Hooked in deploy/systemd/cyberai-daily-scan.service via ExecStartPost.

set -euo pipefail
cd "$(dirname "$0")/.."
. .venv/bin/activate
set -a; . .env; set +a

DATE_TAG=$(date -u +%Y-%m-%d)
TS=$(date -u +%Y%m%d_%H%M%S)
PROSE_FILE=/tmp/daily_summary_${TS}.txt

# 1. Have GLM-5.1 author the Slack body from today's per-target stats
python3 <<'PYEOF' > "$PROSE_FILE"
import os, json, glob, asyncio, sys
sys.path.insert(0, "src")
from cyberai.models.glm import GLMAdapter

# Aggregate today's results
TARGETS = ["libpng","expat","curl","nginx","sqlite","openssl","zlib","libxml2","libssh2","freetype"]
stats = {}
total = {"CRITICAL":0,"HIGH":0,"MEDIUM":0,"LOW":0,"INFO":0,"segments":0}
for t in TARGETS:
    cands = sorted(glob.glob(f"research/{t}/*results*.json"))
    if not cands: continue
    try:
        d = json.load(open(cands[-1]))
    except Exception: continue
    items = d.get("results", d) if isinstance(d, dict) else d
    if not isinstance(items, list): items = [items]
    sev = {"CRITICAL":0,"HIGH":0,"MEDIUM":0,"LOW":0,"INFO":0}
    for r in items:
        if not isinstance(r, dict): continue
        for f in r.get("findings", []):
            s = (f.get("severity") or "?").upper()
            if s in sev: sev[s] += 1
    nfind = sum(sev.values())
    stats[t] = {"segments": len(items), "findings": nfind, **sev}
    total["segments"] += len(items)
    for k in ("CRITICAL","HIGH","MEDIUM","LOW","INFO"):
        total[k] += sev[k]

prompt = f"""Write a short Slack status update for the daily CyberAI scan that just
finished. Plain text. ~80 words. Use 1 emoji max. Include:
- Total raw findings + CRITICAL+HIGH count
- Top 2 targets by CRITICAL count
- One sentence reminder that these are raw findings, not verified

Per-target stats:
{json.dumps(stats, indent=2)}

Aggregate:
{json.dumps(total)}

Output the text only, no preamble."""

async def go():
    api_key = os.environ.get("GLM_API_KEY","")
    g = GLMAdapter(model_name="glm-5.1", api_key=api_key)
    resp, _ = await g.chat([{"role":"user","content":prompt}], timeout=120.0)
    print(resp.strip())

asyncio.run(go())
PYEOF

# 2. POST directly to Slack incoming webhook (no gh CLI dependency)
test -n "${SLACK_WEBHOOK_URL:-}" || { echo "[daily_summary] SLACK_WEBHOOK_URL not set, skipping"; exit 0; }

PROSE=$(cat "$PROSE_FILE")
PAYLOAD=$(python3 -c '
import json, sys, os
prose = open(sys.argv[1]).read().strip()
date_tag = sys.argv[2]
text = f"*CyberAI daily scan — {date_tag}*\n\n{prose}\n\nRaw: oss://cyberai-scan-results-us1/scans/{date_tag}/\nRepo: https://github.com/fxp/cyberai/tree/main/research"
print(json.dumps({
    "attachments": [{
        "color": "good",
        "blocks": [{"type": "section", "text": {"type": "mrkdwn", "text": text}}]
    }]
}))
' "$PROSE_FILE" "$DATE_TAG")

curl -sS -X POST -H 'Content-Type: application/json' --data "$PAYLOAD" "$SLACK_WEBHOOK_URL"
echo
echo "[daily_summary] posted to Slack at $(date -u +%H:%M:%SZ), prose at $PROSE_FILE"
