"""SQLite-based storage for scan results and experiment data.

Uses SQLAlchemy async with aiosqlite for non-blocking database access.
Designed to migrate to PostgreSQL when needed.
"""

from __future__ import annotations

import json
from datetime import datetime, timezone

from sqlalchemy import Column, DateTime, Float, Integer, String, Text, create_engine
from sqlalchemy.ext.asyncio import AsyncSession, create_async_engine
from sqlalchemy.orm import DeclarativeBase, sessionmaker

from cyberai.config import settings


class Base(DeclarativeBase):
    pass


class ScanRecord(Base):
    __tablename__ = "scan_records"

    id = Column(String(16), primary_key=True)
    target = Column(String(500), nullable=False, index=True)
    model = Column(String(100), nullable=False, index=True)
    findings_json = Column(Text, default="[]")
    findings_count = Column(Integer, default=0)
    input_tokens = Column(Integer, default=0)
    output_tokens = Column(Integer, default=0)
    cost_usd = Column(Float, default=0.0)
    duration_seconds = Column(Float, default=0.0)
    error = Column(Text, nullable=True)
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))


class PipelineRun(Base):
    __tablename__ = "pipeline_runs"

    id = Column(String(16), primary_key=True)
    project = Column(String(500), nullable=False)
    model = Column(String(100), nullable=False)
    files_discovered = Column(Integer, default=0)
    files_scanned = Column(Integer, default=0)
    total_findings = Column(Integer, default=0)
    verified_findings = Column(Integer, default=0)
    total_cost_usd = Column(Float, default=0.0)
    total_tokens = Column(Integer, default=0)
    duration_seconds = Column(Float, default=0.0)
    config_json = Column(Text, default="{}")
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))


# ---------------------------------------------------------------------------
# Database session management
# ---------------------------------------------------------------------------

_engine = None
_async_session = None


async def get_engine():
    global _engine
    if _engine is None:
        _engine = create_async_engine(settings.database_url, echo=False)
        async with _engine.begin() as conn:
            await conn.run_sync(Base.metadata.create_all)
    return _engine


async def get_session() -> AsyncSession:
    global _async_session
    if _async_session is None:
        engine = await get_engine()
        _async_session = sessionmaker(engine, class_=AsyncSession, expire_on_commit=False)
    return _async_session()


async def save_scan_result(scan_result) -> None:
    """Persist a ScanResult to the database."""
    from cyberai.models.base import ScanResult as SR
    from dataclasses import asdict

    session = await get_session()
    async with session:
        record = ScanRecord(
            id=scan_result.task_id,
            target=scan_result.target,
            model=scan_result.model,
            findings_json=json.dumps(
                [{"title": f.title, "vuln_type": f.vuln_type, "severity": f.severity.value,
                  "confidence": f.confidence, "description": f.description}
                 for f in scan_result.findings],
                ensure_ascii=False,
            ),
            findings_count=scan_result.vuln_count,
            input_tokens=scan_result.usage.input_tokens,
            output_tokens=scan_result.usage.output_tokens,
            cost_usd=scan_result.usage.cost_usd,
            duration_seconds=scan_result.duration_seconds,
            error=scan_result.error,
        )
        session.add(record)
        await session.commit()
