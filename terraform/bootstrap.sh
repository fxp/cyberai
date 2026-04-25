#!/usr/bin/env bash
# bootstrap.sh — 初次部署前运行，创建 Terraform 状态存储 bucket
# 需要已安装 aliyun CLI 并配置凭证:
#   brew install aliyun-cli
#   aliyun configure  (填入 AccessKey + region=us-west-1)
#
# 运行方式:
#   chmod +x terraform/bootstrap.sh
#   ./terraform/bootstrap.sh

set -euo pipefail

REGION="us-west-1"
TFSTATE_BUCKET="cyberai-tfstate-uswest"
RESULTS_BUCKET="cyberai-scan-results-us1"

echo "🚀 CyberAI Terraform Bootstrap"
echo "   Region:         $REGION"
echo "   TFState bucket: $TFSTATE_BUCKET"
echo "   Results bucket: $RESULTS_BUCKET"
echo ""

# ── 检查 aliyun CLI ────────────────────────────────────────────────
if ! command -v aliyun &>/dev/null; then
  echo "❌ aliyun CLI not found. Install: brew install aliyun-cli"
  exit 1
fi

# ── 检查 terraform ─────────────────────────────────────────────────
if ! command -v terraform &>/dev/null; then
  echo "❌ terraform not found. Install: brew install terraform"
  exit 1
fi

# ── 读取凭证（从 .env 或环境变量）────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/../.env"

if [ -f "$ENV_FILE" ]; then
  export $(grep -E '^ALIYUN_|^ALICLOUD_' "$ENV_FILE" | grep -v '#' | xargs) 2>/dev/null || true
fi

# 兼容两种环境变量名
AK_ID="${ALICLOUD_ACCESS_KEY:-${ALIYUN_ACCESS_KEY_ID:-}}"
AK_SECRET="${ALICLOUD_SECRET_KEY:-${ALIYUN_ACCESS_KEY_SECRET:-}}"

if [ -z "$AK_ID" ] || [ -z "$AK_SECRET" ]; then
  echo "❌ Alibaba Cloud credentials not found."
  echo "   Set ALICLOUD_ACCESS_KEY and ALICLOUD_SECRET_KEY in .env or environment."
  exit 1
fi

export ALICLOUD_ACCESS_KEY="$AK_ID"
export ALICLOUD_SECRET_KEY="$AK_SECRET"
export ALICLOUD_REGION="$REGION"

# ── 创建 TFState bucket（idempotent）─────────────────────────────
echo "1/3 创建 TFState bucket: $TFSTATE_BUCKET"
aliyun oss mb "oss://$TFSTATE_BUCKET" \
  --region "$REGION" \
  --acl private \
  2>&1 | grep -v "BucketAlreadyExists" || true
echo "   ✅ $TFSTATE_BUCKET ready"

# ── 创建 Results bucket（idempotent）────────────────────────────
echo "2/3 创建 Results bucket: $RESULTS_BUCKET"
aliyun oss mb "oss://$RESULTS_BUCKET" \
  --region "$REGION" \
  --acl private \
  2>&1 | grep -v "BucketAlreadyExists" || true
echo "   ✅ $RESULTS_BUCKET ready"

# ── terraform init（local backend，凭证走 env var）────────────────
echo "3/3 terraform init + apply"
cd "$SCRIPT_DIR"
ALICLOUD_ACCESS_KEY="$AK_ID" ALICLOUD_SECRET_KEY="$AK_SECRET" ALICLOUD_REGION="$REGION" \
  NO_PROXY="*" HTTP_PROXY="" HTTPS_PROXY="" http_proxy="" https_proxy="" \
  terraform init -reconfigure
ALICLOUD_ACCESS_KEY="$AK_ID" ALICLOUD_SECRET_KEY="$AK_SECRET" ALICLOUD_REGION="$REGION" \
  NO_PROXY="*" HTTP_PROXY="" HTTPS_PROXY="" http_proxy="" https_proxy="" \
  terraform apply -auto-approve

echo ""
echo "✅ Bootstrap 完成！下一步:"
echo "   cd terraform/"
echo "   terraform plan"
echo "   terraform apply"
echo ""
echo "   apply 完成后，从输出读取 GitHub Actions AK:"
echo "   cat /tmp/cyberai-github-actions-ak.json"
echo ""
echo "   然后在 GitHub repo 添加 Secrets:"
echo "   Settings → Secrets → Actions → New repository secret"
