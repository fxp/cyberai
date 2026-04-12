FROM python:3.12-slim

WORKDIR /app

# Install system deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl && \
    rm -rf /var/lib/apt/lists/*

# Install Python deps
COPY pyproject.toml .
RUN pip install --no-cache-dir -e ".[aliyun]"

# Copy source
COPY src/ src/

EXPOSE 8000

CMD ["uvicorn", "cyberai.api.main:app", "--host", "0.0.0.0", "--port", "8000"]
