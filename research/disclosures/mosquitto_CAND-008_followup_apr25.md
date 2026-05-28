# CAND-008 Follow-up Email — April 25, 2026

> **Send if no response by**: 2026-04-25
> **Status**: READY TO SEND

---

**To**: security@mosquitto.org  
**CC**: security@eclipse.org  
**Subject**: [Follow-up] [Security] MQTT 5.0 property_length uint32_t underflow in property__read() — CPU throughput DoS

---

Hello Mosquitto Security Team,

I'm following up on a vulnerability report I sent on April 18, 2026 regarding an integer underflow in `lib/property_mosq.c: property__read()` when parsing MQTT 5.0 property sections.

**Summary of the issue**:
- A `uint32_t` underflow in the property length counter causes the broker's event loop to process up to ~256,000 phantom property structs for a single malicious packet
- Exploitable pre-authentication via the CONNECT packet (no credentials required)
- Measured latency impact: 20× mean / 33× P99 for legitimate clients under 50 concurrent attackers sending 5 MB packets
- CVSS 3.1 score: **5.3 (Medium)** — `AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:L`

I want to confirm you received the original report. If helpful, I can provide:
- The complete technical write-up with PoC code
- Docker-reproducible test environment
- Proposed patch (bounds check before each byte decrement in `property__read()`)

Our disclosure policy follows the 90-day coordinated disclosure standard. The current deadline is **July 17, 2026**.

Please let me know:
1. Whether you have received and are reviewing the original report
2. Your expected timeline for a fix
3. Whether you prefer a CVE to be requested via MITRE now or once a patch is ready

Looking forward to working with you on this.

Best regards,  
CyberAI Security Research  
fxp007@gmail.com

---

## If No Response by May 2, 2026 → Escalate to Eclipse Security

**To**: security@eclipse.org  
**Subject**: [Escalation] Unacknowledged vulnerability in Eclipse Mosquitto — MQTT 5.0 property underflow

Hello Eclipse Security Team,

I'm writing to escalate an unacknowledged vulnerability report affecting Eclipse Mosquitto 2.0.21.

I sent an initial report to security@mosquitto.org on April 18, 2026 and a follow-up on April 25, 2026, but have received no acknowledgment. In accordance with our 90-day responsible disclosure policy, public disclosure is scheduled for **July 17, 2026**.

The vulnerability is an integer underflow in MQTT 5.0 property parsing (`lib/property_mosq.c`) exploitable pre-authentication, causing measurable CPU throughput degradation under concurrent attack.

Could you please help route this to the appropriate Mosquitto maintainer? I'm happy to provide full technical details upon acknowledgment.

Best regards,  
CyberAI Security Research  
fxp007@gmail.com

---

## Timeline Tracker

| Date | Action | Status |
|------|--------|--------|
| 2026-04-18 | Initial disclosure sent | ✅ Done |
| 2026-04-25 | Follow-up to security@mosquitto.org | ⬜ **PENDING** |
| 2026-05-02 | Escalate to security@eclipse.org | ⬜ Pending |
| 2026-07-17 | Public disclosure deadline | ⬜ Pending |
