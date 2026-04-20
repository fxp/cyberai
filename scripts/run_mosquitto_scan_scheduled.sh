#!/bin/bash
cd /Users/xiaopingfeng/Projects/cyberai
LOG=/Users/xiaopingfeng/Projects/cyberai/research/mosquitto/t1_glm5_scan.log
echo "[$(date)] Starting Mosquitto T1 GLM-5.1 scan..." >> "$LOG"
/opt/homebrew/bin/python3 scripts/scan_mosquitto_t1.py --delay 30 >> "$LOG" 2>&1
echo "[$(date)] Scan finished (exit $?)" >> "$LOG"
