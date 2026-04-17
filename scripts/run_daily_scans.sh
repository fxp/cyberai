#!/usr/bin/env bash
# run_daily_scans.sh — 每日 GLM 扫描批次 (08:05 BJT 触发)
#
# 运行顺序:
#   1. libpng 1.6.45  (v3 extracts, 90s timeout, 14 segments, ~30 min)
#   2. expat 2.6.4    (t1 extracts, 150s timeout, 30 segments, ~90 min)
#   3. curl retry     (segments 0,1,8,9,13, 240s timeout, ~40 min)
#   4. nginx 1.27.4   (t1 extracts, 150s timeout, 33 segments, ~90 min)
#   5. sqlite 3.49.1  (t1 extracts, 150s timeout, 29 segments, ~80 min)
#   6. openssl 3.4.1  (t1 extracts, 150s timeout, 32 segments, ~88 min)
#
# 总计: ~418 min; 预计完成时间 08:05 + 6:58 = 15:03 BJT
#
# Usage: bash scripts/run_daily_scans.sh [--libpng] [--expat] [--curl] [--nginx] [--sqlite] [--openssl] [--all]
#        (no args = --all)

set -euo pipefail
cd "$(dirname "$0")/.."

LOG_DIR="research/scan_logs"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG="$LOG_DIR/daily_${TIMESTAMP}.log"

log() { echo "[$(date '+%H:%M:%S')] $*" | tee -a "$LOG"; }

RUN_LIBPNG=0; RUN_EXPAT=0; RUN_CURL=0; RUN_NGINX=0; RUN_SQLITE=0; RUN_OPENSSL=0

if [[ $# -eq 0 || " $* " == *" --all "* ]]; then
  RUN_LIBPNG=1; RUN_EXPAT=1; RUN_CURL=1; RUN_NGINX=1; RUN_SQLITE=1; RUN_OPENSSL=1
fi
for arg in "$@"; do
  case $arg in
    --libpng)  RUN_LIBPNG=1  ;;
    --expat)   RUN_EXPAT=1   ;;
    --curl)    RUN_CURL=1    ;;
    --nginx)   RUN_NGINX=1   ;;
    --sqlite)  RUN_SQLITE=1  ;;
    --openssl) RUN_OPENSSL=1 ;;
  esac
done

log "=== CyberAI Daily Scan Batch ==="
log "Date: $(date '+%Y-%m-%d %H:%M:%S %Z')"
log "Tasks: libpng=$RUN_LIBPNG expat=$RUN_EXPAT curl=$RUN_CURL nginx=$RUN_NGINX sqlite=$RUN_SQLITE openssl=$RUN_OPENSSL"

# ── 1. libpng ──
if [[ $RUN_LIBPNG -eq 1 ]]; then
  log "--- [1/4] libpng 1.6.45 GLM scan (v3, 14 segments, 90s timeout) ---"
  python scripts/scan_libpng_v3.py --timeout 90 --delay 20 2>&1 | tee -a "$LOG" || \
    log "WARNING: libpng scan exited non-zero"
  log "--- libpng scan done ---"
  sleep 60  # brief cooldown before next target
fi

# ── 2. expat ──
if [[ $RUN_EXPAT -eq 1 ]]; then
  log "--- [2/4] expat 2.6.4 GLM scan (30 segments, 150s timeout) ---"
  python scripts/scan_expat_t1.py --timeout 150 --delay 30 2>&1 | tee -a "$LOG" || \
    log "WARNING: expat scan exited non-zero"
  log "--- expat scan done ---"
  sleep 60
fi

# ── 3. curl retry ──
if [[ $RUN_CURL -eq 1 ]]; then
  log "--- [3/4] curl T1 retry (segments 0,1,8,9,13, 240s timeout) ---"
  python scripts/scan_curl_t1_retry.py \
    --segments 0,1,8,9,13 --timeout 240 --delay 60 2>&1 | tee -a "$LOG" || \
    log "WARNING: curl retry exited non-zero"
  log "--- curl retry done ---"
  sleep 60
fi

# ── 4. nginx ──
if [[ $RUN_NGINX -eq 1 ]]; then
  log "--- [4/5] nginx 1.27.4 GLM scan (t1, 33 segments, 150s timeout) ---"
  python scripts/scan_nginx_t1.py --timeout 150 --delay 30 2>&1 | tee -a "$LOG" || \
    log "WARNING: nginx scan exited non-zero"
  log "--- nginx scan done ---"
  sleep 60
fi

# ── 5. sqlite ──
if [[ $RUN_SQLITE -eq 1 ]]; then
  log "--- [5/6] sqlite 3.49.1 GLM scan (t1, 29 segments, 150s timeout) ---"
  python scripts/scan_sqlite_t1.py --timeout 150 --delay 30 2>&1 | tee -a "$LOG" || \
    log "WARNING: sqlite scan exited non-zero"
  log "--- sqlite scan done ---"
  sleep 60
fi

# ── 6. openssl ──
if [[ $RUN_OPENSSL -eq 1 ]]; then
  log "--- [6/6] openssl 3.4.1 GLM scan (t1, 32 segments, 150s timeout) ---"
  python scripts/scan_openssl_t1.py --timeout 150 --delay 30 2>&1 | tee -a "$LOG" || \
    log "WARNING: openssl scan exited non-zero"
  log "--- openssl scan done ---"
fi

log "=== All scans complete. Log: $LOG ==="
echo "$LOG"
