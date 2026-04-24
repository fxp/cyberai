terraform {
  required_version = ">= 1.6"
  required_providers {
    alicloud = {
      source  = "aliyun/alicloud"
      version = "~> 1.230"
    }
  }

  # ⚠️  先决条件：OSS tfstate bucket 必须已存在。
  #    首次部署请先运行:  ./terraform/bootstrap.sh
  #    它会自动创建 bucket 并执行 terraform init。
  backend "oss" {
    bucket   = "cyberai-tfstate-uswest"
    prefix   = "terraform/state"
    region   = "us-west-1"
    endpoint = "oss-us-west-1.aliyuncs.com"
    # access_key 和 secret_key 通过 bootstrap.sh 的 -backend-config 传入，
    # 不写在此处以避免凭证泄露到 git。
  }
}

provider "alicloud" {
  region = var.region
}

# ─── OSS：扫描结果存储 ────────────────────────────────────────────
resource "alicloud_oss_bucket" "results" {
  bucket        = var.results_bucket
  acl           = "private"
  force_destroy = false

  lifecycle_rule {
    id      = "expire-raw-findings"
    enabled = true
    prefix  = "raw/"

    expiration {
      days = 90 # 原始 findings 保留 90 天
    }
  }

  tags = {
    project = "cyberai"
    env     = var.env
  }
}

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
