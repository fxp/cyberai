terraform {
  required_version = ">= 1.6"
  required_providers {
    alicloud = {
      source  = "aliyun/alicloud"
      version = "~> 1.230"
    }
  }

  # State 存本地（单人项目，terraform.tfstate 加入 .gitignore）
  # 如需远程 state：参考 bootstrap.sh 中的 OSS backend 配置说明
}

provider "alicloud" {
  region = var.region
}

# ─── OSS bucket 由 bootstrap.sh 通过 aliyun CLI 创建 ────────────
# terraform alicloud provider 在境外访问 us-west-1 OSS 时使用 internal
# endpoint，无法从本地 import/apply OSS 资源。bucket 手动管理即可：
#   aliyun oss mb oss://cyberai-scan-results-us1 --region us-west-1 --acl private

# ─── RAM：GitHub Actions 专用账号 ────────────────────────────────
resource "alicloud_ram_user" "github_actions" {
  name         = "cyberai-github-actions"
  display_name = "CyberAI GitHub Actions CI"
  comments     = "Used by GitHub Actions to upload scan results to OSS"
}

resource "alicloud_ram_policy" "oss_write" {
  policy_name     = "cyberai-oss-write"
  policy_document = jsonencode({
    Version = "1"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "oss:PutObject",
          "oss:GetObject",
          "oss:ListObjects",
          "oss:DeleteObject",
        ]
        Resource = [
          "acs:oss:*:*:${var.results_bucket}",
          "acs:oss:*:*:${var.results_bucket}/*",
        ]
      }
    ]
  })
}

resource "alicloud_ram_user_policy_attachment" "github_oss" {
  policy_name = alicloud_ram_policy.oss_write.policy_name
  policy_type = alicloud_ram_policy.oss_write.type
  user_name   = alicloud_ram_user.github_actions.name
}

resource "alicloud_ram_access_key" "github_actions" {
  user_name = alicloud_ram_user.github_actions.name
  secret_file = "/tmp/cyberai-github-actions-ak.json" # 本地保存，手动加到 GitHub Secrets
}
