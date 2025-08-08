#!/usr/bin/env bash

# This script automates the build process for the spooky_scoreboard_daemon (ssbd)
# binary inside a Docker container for a specified game and Linux distribution
# (e.g., Arch Linux or Debian).
#
# Usage: ./build.sh <game_name> <distribution>
# Example: ./build.sh hwn arch
# Example: ./build.sh tcm debian

# Enable strict mode
set -euo pipefail
IFS=$'\n\t'

# Helper functions
log_info() {
  echo "INFO: $1"
}

log_success() {
  echo "SUCCESS: $1"
}

log_error() {
  echo "ERROR: $1" >&2
}

# Check if docker is installed
if ! command -v docker &> /dev/null; then
  log_error "Docker is not installed. Please install Docker first."
  exit 1
fi

# Check if correct number of arguments are provided
if [ $# -ne 2 ]; then
  log_error "Invalid number of arguments"
  echo "Usage: $0 <game_name> <distribution>"
  echo "Example: $0 hwn arch"
  echo "Example: $0 tcm debian"
  exit 1
fi

readonly GAME_NAME=$1
readonly DISTRO=$2

log_info "Current branch: $(git rev-parse --abbrev-ref HEAD)"
log_info "Current commit: $(git rev-parse HEAD)"

# Validate distribution
if [ ! -d "$DISTRO" ]; then
  log_error "Distribution: '$DISTRO' not found"
  log_info "Available distributions:"
  ls -d */
  exit 1
fi

# Validate game directory exists
if [ ! -d "$DISTRO/$GAME_NAME" ]; then
  log_error "Game '$GAME_NAME' not found in $DISTRO directory"
  log_info "Available games in $DISTRO:"
  ls -d "$DISTRO"/*/
  exit 1
fi

# Validate Dockerfile exists
if [ ! -f "$DISTRO/$GAME_NAME/Dockerfile" ]; then
  log_error "Dockerfile not found in $DISTRO/$GAME_NAME/"
  exit 1
fi

# Use a unique tag with commit SHA to avoid caching issues
readonly COMMIT_SHA=$(git rev-parse --short HEAD)
readonly IMAGE_TAG="${GAME_NAME}:${DISTRO}-${COMMIT_SHA}"

# Build the Docker image, using the repository root as context
log_info "Building Docker image for $GAME_NAME using $DISTRO..."
docker build --no-cache -t "$IMAGE_TAG" -f "$DISTRO/$GAME_NAME/Dockerfile" ../

# Check if build was successful
if [ $? -eq 0 ]; then
  log_success "Successfully built Docker image: $IMAGE_TAG"
else
  log_error "Docker build failed"
  exit 1
fi

# Create temporary container to copy files
log_info "Creating temporary container to copy built binary..."
CONTAINER_ID=$(docker create "$IMAGE_TAG")

# Create dist directory if it doesn't exist
mkdir -p "../dist/$DISTRO"

# Copy the binary from container to dist directory
log_info "Copying binary to dist/$DISTRO/ssbd..."
docker cp "$CONTAINER_ID:/ssbd/build/ssbd" "../dist/$DISTRO/ssbd"

# Remove temporary container
log_info "Cleaning up temporary container..."
docker rm "$CONTAINER_ID" > /dev/null

# Check if copy was successful
if [ -f "../dist/$DISTRO/ssbd" ]; then
  log_success "Successfully copied binary to dist/$DISTRO/ssbd"
else
  log_error "Failed to copy binary"
  exit 1
fi

# Print summary
echo
log_success "Build process completed successfully!"
log_info "Binary location: dist/$DISTRO/ssbd"
