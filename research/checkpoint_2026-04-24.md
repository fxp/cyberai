# CyberAI 研究进度快照
**时间**: 2026-04-24  
**状态**: Phase 2 扫描完成，已暂停

## 已完成
- Phase 1: Mosquitto CAND-008 (uint32_t underflow) — 已披露 2026-04-18
- Phase 2 扫描: 6 个目标，1328 findings，全部验证为 FP
  - linux_bpf: 260 findings (research/linux_bpf/t1_glm_phase2_2316.jsonl)
  - linux_kvm: 204 findings (research/linux_kvm/t1_glm_phase2_2316.jsonl)
  - linux_usb: 110 findings (research/linux_usb/t1_glm_phase2_2316.jsonl)
  - imagemagick: 431 findings (research/imagemagick/t1_glm_phase2_2317.jsonl)
  - poppler: 294 findings (research/poppler/t1_glm_phase2_2317.jsonl)
  - libxml2: 29 findings (research/libxml2/t1_glm_phase2_2317.jsonl)
- 二次验证: scripts/verify_candidates.py，30 候选，0 真实漏洞
- 报告: research/triage_report_phase2.md
- 博客: research/blog_phase2_postmortem.md
- 网站: https://fxp.github.io/cyberai/

## 待继续
- libxml2 xpath_name_A.c UTF-8 OOB — PoC 构造
- Pipeline B 设计（函数级上下文扫描）
- 可选新目标: nginx/openssl/sqlite (extracts 已存在)

## 环境状态
- GLM API: ✅ 可用
- Docker: ❌ 未运行
- 磁盘: 23GB 剩余
- Python: 3.14.2 + zhipuai 2.1.5
