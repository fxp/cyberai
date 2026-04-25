output "results_bucket_name" {
  value       = var.results_bucket
  description = "OSS bucket for scan results (created via bootstrap.sh)"
}

output "results_bucket_endpoint" {
  value       = "https://${var.results_bucket}.oss-${var.region}.aliyuncs.com"
  description = "OSS bucket endpoint"
}

output "github_actions_user" {
  value       = alicloud_ram_user.github_actions.name
  description = "RAM user for GitHub Actions"
}

output "github_secrets_instructions" {
  value = <<-EOT
    Add these secrets to your GitHub repo (Settings → Secrets → Actions):

    ALIYUN_ACCESS_KEY_ID      → from /tmp/cyberai-github-actions-ak.json
    ALIYUN_ACCESS_KEY_SECRET  → from /tmp/cyberai-github-actions-ak.json
    GLM_API_KEY               → your BigModel API key
    ANTHROPIC_API_KEY         → your Anthropic API key (for Pipeline B)
    ALIYUN_OSS_BUCKET         → ${var.results_bucket}
  EOT
  description = "Instructions for setting GitHub Secrets"
}
