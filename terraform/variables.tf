variable "region" {
  description = "Alibaba Cloud region"
  type        = string
  default     = "us-west-1"
}

variable "env" {
  description = "Environment tag (dev / prod)"
  type        = string
  default     = "prod"
}

variable "results_bucket" {
  description = "OSS bucket name for scan results"
  type        = string
  default     = "cyberai-scan-results-us1"
}
