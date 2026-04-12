"""Alibaba Cloud OSS client for storing scan results and reports."""

from __future__ import annotations

import json
import logging
from pathlib import Path

from cyberai.config import settings

logger = logging.getLogger(__name__)


class OSSClient:
    """Wrapper around oss2 SDK for result storage."""

    def __init__(self):
        self._bucket = None

    @property
    def bucket(self):
        if self._bucket is None:
            import oss2

            auth = oss2.Auth(
                settings.aliyun_access_key_id,
                settings.aliyun_access_key_secret,
            )
            self._bucket = oss2.Bucket(
                auth,
                settings.aliyun_oss_endpoint,
                settings.aliyun_oss_bucket,
            )
        return self._bucket

    def upload_json(self, key: str, data: dict) -> str:
        """Upload a JSON object to OSS.

        Args:
            key: Object key (path in bucket), e.g. "scans/2024/result_abc.json"
            data: Dict to serialize as JSON.

        Returns:
            Full OSS URL of the uploaded object.
        """
        content = json.dumps(data, ensure_ascii=False, indent=2)
        self.bucket.put_object(key, content.encode("utf-8"))
        url = f"oss://{settings.aliyun_oss_bucket}/{key}"
        logger.info("Uploaded to %s", url)
        return url

    def upload_file(self, key: str, local_path: str | Path) -> str:
        """Upload a local file to OSS."""
        self.bucket.put_object_from_file(key, str(local_path))
        return f"oss://{settings.aliyun_oss_bucket}/{key}"

    def download_json(self, key: str) -> dict:
        """Download and parse a JSON object from OSS."""
        result = self.bucket.get_object(key)
        return json.loads(result.read().decode("utf-8"))

    def list_objects(self, prefix: str = "", max_keys: int = 100) -> list[str]:
        """List object keys under a prefix."""
        import oss2

        keys = []
        for obj in oss2.ObjectIterator(self.bucket, prefix=prefix, max_keys=max_keys):
            keys.append(obj.key)
        return keys
