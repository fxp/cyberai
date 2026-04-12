#!/bin/bash
# ECS instance initialization script
# Run this on a fresh Ubuntu 22.04 ECS instance

set -euo pipefail

echo "=== CyberAI ECS Setup ==="

# Update system
sudo apt-get update && sudo apt-get upgrade -y

# Install Docker
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt-get install -y docker-compose-plugin

# Install Python 3.12
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get install -y python3.12 python3.12-venv python3.12-dev

# Install build tools (for scanning targets)
sudo apt-get install -y build-essential gcc g++ make cmake git curl wget

# Create project directory
mkdir -p ~/cyberai/data
cd ~/cyberai

echo "=== Setup complete ==="
echo "Next steps:"
echo "1. Clone/upload the cyberai code to ~/cyberai/"
echo "2. Copy .env.example to .env and fill in API keys"
echo "3. Run: docker compose up -d"
