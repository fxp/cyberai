# CyberAI Terraform Infrastructure

管理 CyberAI 所需的最小云基础设施：OSS 结果存储 + GitHub Actions 专用 RAM 账号。

**扫描计算本身不在这里** — 计算发生在 GitHub Actions runner（免费）和 LLM API 侧。

## 架构

```
GitHub Actions (免费 runner)
    │  clone 目标 repo
    │  调用 GLM / Claude API  ← 真正的"扫描计算"
    │  上传结果
    ▼
阿里云 OSS (cyberai-results-uswest)  ← Terraform 管理
    └── raw/{target}/{run_id}/*.jsonl
    └── pipeline-b/{run_id}/*.jsonl
```

## 前置条件

```bash
brew install terraform
# 配置阿里云凭证（用主账号 AK 执行 terraform，之后 GitHub Actions 用专用子账号）
export ALICLOUD_ACCESS_KEY="your-main-ak-id"
export ALICLOUD_SECRET_KEY="your-main-ak-secret"
export ALICLOUD_REGION="us-west-1"
```

## 初次部署

```bash
cd terraform/

# 1. 初始化（首次需要先手动创建 tfstate bucket，或改用 local backend）
#    如果 OSS tfstate bucket 不存在，先临时用 local backend：
#    注释掉 main.tf 中的 backend "oss" 块，运行完再迁移

terraform init
terraform plan
terraform apply

# 2. apply 完成后，从输出读取 GitHub Actions AK
cat /tmp/cyberai-github-actions-ak.json
```

## 把 AK 加到 GitHub Secrets

```
GitHub → fxp/cyberai → Settings → Secrets → Actions → New repository secret

ALIYUN_ACCESS_KEY_ID      ← 从 /tmp/cyberai-github-actions-ak.json 读取
ALIYUN_ACCESS_KEY_SECRET  ← 从 /tmp/cyberai-github-actions-ak.json 读取
GLM_API_KEY               ← BigModel API key
ANTHROPIC_API_KEY         ← Claude API key（Pipeline B 用）
ALIYUN_OSS_BUCKET         ← cyberai-results-uswest
```

## 触发扫描

Pipeline A（GLM 批量扫描）：
```
GitHub → Actions → Pipeline A — GLM Batch Scan → Run workflow
```

Pipeline B（Agentic 深度扫描）：
```
GitHub → Actions → Pipeline B — Agentic Scan → Run workflow
  target_repo: https://github.com/eclipse/mosquitto
  model: claude-opus-4-6
  max_files: 50
```

## 费用估算

| 组件 | 月费 |
|------|------|
| GitHub Actions（公开 repo）| 免费 |
| 阿里云 OSS（~1GB 结果）| ¥1-2 |
| LLM API（Pipeline A, GLM-4-flash）| ¥20-50/次扫描 |
| LLM API（Pipeline B, Claude Opus）| $30-100/次扫描（<$30/文件） |
| ECS 服务器 | **¥0**（取消，改用 GitHub Actions） |

总计：接近零固定成本，按扫描次数付费。
