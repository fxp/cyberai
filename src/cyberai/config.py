"""Centralized configuration management."""

from pydantic_settings import BaseSettings
from pydantic import Field


class Settings(BaseSettings):
    """Application settings loaded from environment variables / .env file."""

    # --- LLM API Keys ---
    glm_api_key: str = Field(default="", alias="GLM_API_KEY")
    anthropic_api_key: str = Field(default="", alias="ANTHROPIC_API_KEY")
    openai_api_key: str = Field(default="", alias="OPENAI_API_KEY")

    # --- Alibaba Cloud ---
    aliyun_access_key_id: str = Field(default="", alias="ALIYUN_ACCESS_KEY_ID")
    aliyun_access_key_secret: str = Field(default="", alias="ALIYUN_ACCESS_KEY_SECRET")
    aliyun_region: str = Field(default="us-west-1", alias="ALIYUN_REGION")
    aliyun_oss_bucket: str = Field(default="cyberai-results-uswest", alias="ALIYUN_OSS_BUCKET")
    aliyun_oss_endpoint: str = Field(
        default="https://oss-us-west-1.aliyuncs.com", alias="ALIYUN_OSS_ENDPOINT"
    )

    # --- Database ---
    database_url: str = Field(
        default="sqlite+aiosqlite:///./cyberai.db", alias="DATABASE_URL"
    )

    # --- Redis ---
    redis_url: str = Field(default="redis://localhost:6379/0", alias="REDIS_URL")

    # --- API Server ---
    api_host: str = Field(default="0.0.0.0", alias="API_HOST")
    api_port: int = Field(default=8000, alias="API_PORT")

    # --- Scanner ---
    scanner_default_model: str = Field(default="glm-5.1", alias="SCANNER_DEFAULT_MODEL")
    scanner_max_parallel: int = Field(default=4, alias="SCANNER_MAX_PARALLEL")
    scanner_token_budget: int = Field(default=100_000, alias="SCANNER_TOKEN_BUDGET")
    scanner_time_budget: int = Field(default=600, alias="SCANNER_TIME_BUDGET")

    model_config = {"env_file": ".env", "env_file_encoding": "utf-8", "extra": "ignore"}


settings = Settings()
