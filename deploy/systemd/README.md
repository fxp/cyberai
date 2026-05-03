# Systemd units for CyberAI ECS

These units run the daily scan batch on a fresh Ubuntu ECS that's already
been provisioned via `deploy/setup_ecs.sh` (Docker + Python via uv).

## Install

```bash
sudo cp deploy/systemd/cyberai-daily-scan.{service,timer} /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now cyberai-daily-scan.timer

# verify
systemctl list-timers cyberai-daily-scan.timer --no-pager

# also lock the server timezone so OnCalendar is unambiguous
sudo timedatectl set-timezone UTC
```

## Schedule

Runs daily at **00:05 UTC** (= 08:05 Beijing time). RandomizedDelaySec=300
spreads start by up to 5 min so multiple machines don't hammer the GLM API
at the same instant.

## Logs

```bash
tail -f /var/log/cyberai-daily-scan.log
journalctl -u cyberai-daily-scan.service -f
```

## Manual fire (skip waiting for timer)

```bash
sudo systemctl start cyberai-daily-scan.service
```
