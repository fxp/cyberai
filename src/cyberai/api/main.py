"""FastAPI application entry point."""

from fastapi import FastAPI
from cyberai.api.routes.scan import router as scan_router
from cyberai.api.routes.results import router as results_router

app = FastAPI(
    title="CyberAI",
    description="AI-powered software security research platform",
    version="0.1.0",
)

app.include_router(scan_router, prefix="/api", tags=["scan"])
app.include_router(results_router, prefix="/api", tags=["results"])


@app.get("/health")
async def health():
    return {"status": "ok", "version": "0.1.0"}


if __name__ == "__main__":
    import uvicorn
    from cyberai.config import settings

    uvicorn.run(app, host=settings.api_host, port=settings.api_port)
