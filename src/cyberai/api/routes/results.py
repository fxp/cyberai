"""Results query API routes."""

from fastapi import APIRouter

from cyberai.models.registry import ModelRegistry

router = APIRouter()


@router.get("/models")
async def list_models():
    """List all available model adapters."""
    return {"models": ModelRegistry.list_models()}


@router.get("/results")
async def list_results():
    """List recent scan results (placeholder — reads from DB)."""
    # TODO: implement DB query
    return {"results": [], "message": "DB query not yet implemented"}
