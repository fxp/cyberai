"""Scan task API routes."""

from __future__ import annotations

import asyncio
import logging
from pathlib import Path
from uuid import uuid4

from fastapi import APIRouter, BackgroundTasks, HTTPException
from pydantic import BaseModel

from cyberai.config import settings
from cyberai.models.registry import ModelRegistry
from cyberai.scanner.carlini import CarliniPipeline, PipelineConfig, PipelineResult
from cyberai.tracker.cost import CostTracker

logger = logging.getLogger(__name__)
router = APIRouter()

# In-memory task store (replace with Redis for production)
_tasks: dict[str, dict] = {}
_tracker = CostTracker()


class ScanRequest(BaseModel):
    """Request to start a vulnerability scan."""
    project_path: str  # local path on server to scan
    model: str = "glm-5.1"
    max_files: int | None = None
    max_parallel: int = 4
    prompt_strategy: str = "ctf"  # "ctf" or "audit"
    verify: bool = True


class ScanFileRequest(BaseModel):
    """Request to scan a single file content."""
    file_content: str
    file_name: str = "untitled.c"
    model: str = "glm-5.1"


class ScanResponse(BaseModel):
    task_id: str
    status: str
    message: str


@router.post("/scan", response_model=ScanResponse)
async def start_scan(req: ScanRequest, background_tasks: BackgroundTasks):
    """Start a Carlini pipeline scan on a project directory."""
    project_path = Path(req.project_path)
    if not project_path.is_dir():
        raise HTTPException(400, f"Directory not found: {req.project_path}")

    task_id = uuid4().hex[:16]
    _tasks[task_id] = {"status": "running", "result": None, "error": None}

    background_tasks.add_task(_run_pipeline, task_id, req)
    return ScanResponse(task_id=task_id, status="running", message="Scan started")


@router.post("/scan/file")
async def scan_single_file(req: ScanFileRequest):
    """Scan a single file content directly (synchronous)."""
    agent = ModelRegistry.create(req.model, api_key=settings.glm_api_key)
    result = await agent.scan_file(req.file_content, req.file_name)

    _tracker.record(
        result.usage,
        task_type="scan_file",
        target=req.file_name,
        experiment_id=result.task_id,
        duration=result.duration_seconds,
        result=f"{result.vuln_count} findings",
    )

    return {
        "task_id": result.task_id,
        "target": result.target,
        "model": result.model,
        "findings": [
            {
                "id": f.id,
                "title": f.title,
                "vuln_type": f.vuln_type,
                "severity": f.severity.value,
                "line_start": f.line_start,
                "line_end": f.line_end,
                "description": f.description,
                "proof_of_concept": f.proof_of_concept,
                "cvss_score": f.cvss_score,
                "confidence": f.confidence,
            }
            for f in result.findings
        ],
        "usage": {
            "input_tokens": result.usage.input_tokens,
            "output_tokens": result.usage.output_tokens,
            "cost_usd": result.usage.cost_usd,
            "latency_ms": result.usage.latency_ms,
        },
        "duration_seconds": round(result.duration_seconds, 2),
    }


@router.get("/scan/{task_id}")
async def get_scan_status(task_id: str):
    """Get the status of a scan task."""
    if task_id not in _tasks:
        raise HTTPException(404, f"Task {task_id} not found")

    task = _tasks[task_id]
    response = {"task_id": task_id, "status": task["status"]}

    if task["result"]:
        r: PipelineResult = task["result"]
        response.update({
            "project": r.project,
            "files_discovered": r.total_files_discovered,
            "files_scanned": r.total_files_scanned,
            "total_findings": r.total_findings,
            "verified_findings": r.total_findings_verified,
            "cost_usd": round(r.total_cost_usd, 6),
            "total_tokens": r.total_tokens,
            "duration_seconds": round(r.duration_seconds, 2),
        })

    if task["error"]:
        response["error"] = task["error"]

    return response


@router.get("/tracker/summary")
async def tracker_summary():
    """Get cost tracking summary."""
    return _tracker.summary()


async def _run_pipeline(task_id: str, req: ScanRequest) -> None:
    """Background task: run the Carlini pipeline."""
    try:
        agent = ModelRegistry.create(req.model, api_key=settings.glm_api_key)
        config = PipelineConfig(
            max_parallel=req.max_parallel,
            max_files=req.max_files,
            prompt_strategy=req.prompt_strategy,
            verify=req.verify,
        )
        pipeline = CarliniPipeline(agent=agent, config=config)
        result = await pipeline.run(req.project_path)

        _tasks[task_id]["status"] = "completed"
        _tasks[task_id]["result"] = result

        # Track costs
        for sr in result.scan_results:
            _tracker.record(
                sr.usage,
                task_type="scan",
                target=sr.target,
                experiment_id=task_id,
                duration=sr.duration_seconds,
                result=f"{sr.vuln_count} findings",
            )

    except Exception as e:
        logger.error("Pipeline failed for task %s: %s", task_id, e)
        _tasks[task_id]["status"] = "failed"
        _tasks[task_id]["error"] = str(e)
