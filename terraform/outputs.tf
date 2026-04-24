output "results_bucket_name" {
  value       = alicloud_oss_bucket.results.bucket
  description = "OSS bucket for scan results"
}

output "results_bucket_endpoint" {
  value       = "https://${alicloud_oss_bucket.results.bucket}.oss-${var.region}.aliyuncs.com"
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
    ALIYUN_OSS_BUCKET         → ${alicloud_oss_bucket.results.bucket}
  EOT
  description = "Instructions for setting GitHub Secrets"
}
