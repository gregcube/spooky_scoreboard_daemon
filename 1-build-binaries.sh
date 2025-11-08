#!/usr/bin/env bash

#
# build-all.sh - Master build script for Spooky Scoreboard Daemon
#
# This script orchestrates the complete build process for all supported games
# and distributions, creating a distribution package ready for installation.
#
# Usage: ./build-all.sh [OPTIONS]
#
# OPTIONS:
#   --clean     Remove .build/dist/ directory before building
#   --test      Test all binaries after building
#   --help      Show this help message
#
# Output: .build/dist/ directory containing all binaries and installer
#

set -euo pipefail

# Constants
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly VERSION_FILE="${SCRIPT_DIR}/VERSION"
readonly DOCKER_BUILD_SCRIPT="${SCRIPT_DIR}/docker/build.sh"
readonly DIST_OUTPUT="${SCRIPT_DIR}/.build/dist"
readonly TEMP_DIR="${SCRIPT_DIR}/.build-temp"

# Build matrix: game -> distribution mapping
# Format: "game:distro" pairs
readonly BUILD_MATRIX=(
  # "hwn:arch"      # Halloween on Arch Linux - TEMPORARILY DISABLED (archive.archlinux.org having 500 errors)
  "tcm:debian"    # Texas Chainsaw Massacre on Debian
  "ed:debian"     # Evil Dead on Debian
)

# Color codes for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Flags
FLAG_CLEAN=false
FLAG_TEST=false

# ============================================================================
# Helper Functions
# ============================================================================

# Print colored log messages
log_info() {
  echo -e "${BLUE}INFO:${NC} $1"
}

log_success() {
  echo -e "${GREEN}SUCCESS:${NC} $1"
}

log_warning() {
  echo -e "${YELLOW}WARNING:${NC} $1"
}

log_error() {
  echo -e "${RED}ERROR:${NC} $1" >&2
}

# Print section headers for clarity
log_section() {
  echo ""
  echo -e "${BLUE}========================================${NC}"
  echo -e "${BLUE}$1${NC}"
  echo -e "${BLUE}========================================${NC}"
}

# Show usage information
show_help() {
  cat << EOF
Spooky Scoreboard Daemon - Master Build Script

USAGE:
  ./build-all.sh [OPTIONS]

OPTIONS:
  --clean     Remove .build/dist/ directory before building
  --test      Test all binaries after building
  --help      Show this help message

DESCRIPTION:
  Builds all game-specific binaries using Docker and packages them
  for distribution. Creates a complete distribution package in
  .build/dist/ ready for USB installer creation.

EXAMPLES:
  ./build-all.sh              # Incremental build
  ./build-all.sh --clean      # Clean build
  ./build-all.sh --test       # Build and test
  ./build-all.sh --clean --test  # Clean, build, and test

OUTPUT:
  .build/dist/
  ├── README.txt
  ├── installer/
  │   └── install.sh
  └── spooky_scoreboard_daemon/
      └── dist/
          ├── arch/
          │   └── ssbd-VERSION-arch-x86_64.tar.gz
          └── debian/
              └── ssbd-VERSION-debian-x86_64.tar.gz

EOF
}

# ============================================================================
# Validation Functions
# ============================================================================

# Validate VERSION file exists and has correct format
# Expected format: X.Y.Z-N (e.g., 0.1.1-1)
validate_version_file() {
  log_info "Validating VERSION file..."

  if [[ ! -f "$VERSION_FILE" ]]; then
    log_error "VERSION file not found at: $VERSION_FILE"
    log_info "Please create a VERSION file in the project root with format: X.Y.Z-N"
    exit 1
  fi

  VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')

  if [[ -z "$VERSION" ]]; then
    log_error "VERSION file is empty"
    exit 1
  fi

  # Validate version format: X.Y.Z-N
  if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+-[0-9]+$ ]]; then
    log_error "Invalid VERSION format: '$VERSION'"
    log_info "Expected format: X.Y.Z-N (e.g., 0.1.1-1)"
    exit 1
  fi

  log_success "Version: $VERSION"
}

# Check if Docker is installed and running
validate_docker() {
  log_info "Checking Docker availability..."

  if ! command -v docker &> /dev/null; then
    log_error "Docker is not installed"
    log_info "Please install Docker: https://docs.docker.com/get-docker/"
    exit 1
  fi

  if ! docker info &> /dev/null; then
    log_error "Docker daemon is not running"
    log_info "Please start Docker and try again"
    exit 1
  fi

  log_success "Docker is available"
}

# Validate docker build script exists
validate_build_script() {
  log_info "Validating build script..."

  if [[ ! -f "$DOCKER_BUILD_SCRIPT" ]]; then
    log_error "Docker build script not found: $DOCKER_BUILD_SCRIPT"
    exit 1
  fi

  if [[ ! -x "$DOCKER_BUILD_SCRIPT" ]]; then
    log_error "Docker build script is not executable: $DOCKER_BUILD_SCRIPT"
    log_info "Run: chmod +x $DOCKER_BUILD_SCRIPT"
    exit 1
  fi

  log_success "Build script validated"
}

# ============================================================================
# Setup Functions
# ============================================================================

# Parse command line arguments
parse_arguments() {
  while [[ $# -gt 0 ]]; do
    case $1 in
      --clean)
        FLAG_CLEAN=true
        shift
        ;;
      --test)
        FLAG_TEST=true
        shift
        ;;
      --help|-h)
        show_help
        exit 0
        ;;
      *)
        log_error "Unknown option: $1"
        echo ""
        show_help
        exit 1
        ;;
    esac
  done
}

# Clean output directory if --clean flag is set
clean_output() {
  if [[ "$FLAG_CLEAN" == true ]]; then
    log_section "Cleaning Output Directory"

    if [[ -d "$DIST_OUTPUT" ]]; then
      log_info "Removing $DIST_OUTPUT..."
      rm -rf "$DIST_OUTPUT"
      log_success "Output directory cleaned"
    else
      log_info "Output directory does not exist, nothing to clean"
    fi
  fi
}

# Create output directory structure
# This mirrors the structure expected by install.sh
create_output_structure() {
  log_section "Creating Output Structure"

  log_info "Creating directories..."
  mkdir -p "$DIST_OUTPUT/installer"
  mkdir -p "$DIST_OUTPUT/spooky_scoreboard_daemon/dist/arch"
  mkdir -p "$DIST_OUTPUT/spooky_scoreboard_daemon/dist/debian"
  mkdir -p "$TEMP_DIR"

  log_success "Directory structure created"
}

# ============================================================================
# Build Functions
# ============================================================================

# Build a single game binary using Docker
# Args: game, distro
build_binary() {
  local game=$1
  local distro=$2
  local tarball_name="ssbd-${VERSION}-${distro}-x86_64.tar.gz"
  local tarball_path="${DIST_OUTPUT}/spooky_scoreboard_daemon/dist/${distro}/${tarball_name}"

  log_info "Building $game for $distro..."

  # Check if tarball already exists
  if [[ -f "$tarball_path" ]]; then
    log_warning "Tarball already exists: $tarball_path"
    log_info "Skipping build (use --clean to rebuild)"
    return 0
  fi

  # Run docker build script
  # Note: build.sh outputs to dist/${distro}/ssbd
  log_info "Running docker build..."
  cd "${SCRIPT_DIR}/docker"
  if ! ./build.sh "$game" "$distro"; then
    log_error "Docker build failed for $game ($distro)"
    exit 1
  fi
  cd "${SCRIPT_DIR}"

  # Verify binary was created
  local binary_path="${SCRIPT_DIR}/dist/${distro}/ssbd"
  if [[ ! -f "$binary_path" ]]; then
    log_error "Binary not found after build: $binary_path"
    exit 1
  fi

  log_success "Binary built successfully"
}

# Package binary as tarball
# Args: distro
package_binary() {
  local distro=$1
  local binary_path="${SCRIPT_DIR}/dist/${distro}/ssbd"
  local tarball_name="ssbd-${VERSION}-${distro}-x86_64.tar.gz"
  local tarball_path="${DIST_OUTPUT}/spooky_scoreboard_daemon/dist/${distro}/${tarball_name}"

  # Skip if tarball already exists
  if [[ -f "$tarball_path" ]]; then
    log_info "Tarball already exists, skipping packaging"
    return 0
  fi

  log_info "Packaging binary as tarball..."

  # Create tarball containing just the ssbd binary
  # -C changes to directory before adding files
  # This ensures tarball extracts as "ssbd" not "dist/debian/ssbd"
  tar -czf "$tarball_path" -C "${SCRIPT_DIR}/dist/${distro}" ssbd

  # Verify tarball was created
  if [[ ! -f "$tarball_path" ]]; then
    log_error "Failed to create tarball: $tarball_path"
    exit 1
  fi

  # Clean up source binary (keep only tarball)
  rm -f "$binary_path"

  log_success "Tarball created: $(basename "$tarball_path")"
}

# Build all binaries in the build matrix
build_all_binaries() {
  log_section "Building All Binaries"

  # Track which distros we've built (avoid duplicate builds for same distro)
  local built_distros=()

  for item in "${BUILD_MATRIX[@]}"; do
    # Parse game:distro pair
    local game="${item%%:*}"
    local distro="${item##*:}"

    echo ""
    log_info "Processing: $game ($distro)"

    # Build binary (skips if tarball exists)
    build_binary "$game" "$distro"

    # Package binary (only if not already packaged for this distro)
    local already_built=false
    for built in "${built_distros[@]}"; do
      if [[ "$built" == "$distro" ]]; then
        already_built=true
        break
      fi
    done

    if [[ "$already_built" == false ]]; then
      package_binary "$distro"
      built_distros+=("$distro")
    else
      log_info "Binary already packaged for $distro, skipping"
    fi

    log_success "$game build complete"
  done

  echo ""
  log_success "All binaries built and packaged"
}

# ============================================================================
# Distribution Package Functions
# ============================================================================

# Copy installer script and VERSION file to distribution package
copy_installer() {
  log_section "Preparing Distribution Package"

  log_info "Copying installer script..."

  local installer_src="${SCRIPT_DIR}/installer/install.sh"
  local installer_dst="${DIST_OUTPUT}/installer/install.sh"

  if [[ ! -f "$installer_src" ]]; then
    log_error "Installer script not found: $installer_src"
    exit 1
  fi

  cp "$installer_src" "$installer_dst"
  chmod +x "$installer_dst"

  log_success "Installer copied"

  # Copy VERSION file so install.sh can read it
  log_info "Copying VERSION file..."
  cp "$VERSION_FILE" "${DIST_OUTPUT}/VERSION"
  log_success "VERSION file copied"
}

# Create README file in distribution package
create_readme() {
  log_info "Creating README..."

  local readme_path="${DIST_OUTPUT}/README.txt"

  cat > "$readme_path" << EOF
Spooky Scoreboard Daemon - Distribution Package
================================================

Version: ${VERSION}
Built: $(date)

CONTENTS
--------
This directory contains pre-built binaries for all supported games
and the installation script required to deploy to pinball machines.

  installer/
    install.sh - Installation script for target machines

  spooky_scoreboard_daemon/dist/
    arch/
      ssbd-${VERSION}-arch-x86_64.tar.gz    - Halloween binary
    debian/
      ssbd-${VERSION}-debian-x86_64.tar.gz  - TCM & Evil Dead binary

INSTALLATION
------------
For complete installation instructions, see:
  ../INSTALL.md

SUPPORTED GAMES
---------------
- Halloween (UP board, Arch Linux)
- Texas Chainsaw Massacre (UP board, Debian)
- Evil Dead (miniPC, Debian)

BUILDING USB INSTALLER
----------------------
This distribution package contains the binaries and installer script.
To create a bootable USB installer, use the separate image creation
script (see installer/README for details).

EOF

  log_success "README created"
}

# ============================================================================
# Testing Functions
# ============================================================================

# Test a single binary by extracting and running -h
# Args: tarball_path, description
test_binary() {
  local tarball_path=$1
  local description=$2

  log_info "Testing $description..."

  # Create temp directory for extraction
  local test_dir="${TEMP_DIR}/test-$(basename "$tarball_path" .tar.gz)"
  mkdir -p "$test_dir"

  # Extract binary
  if ! tar -xzf "$tarball_path" -C "$test_dir" 2>/dev/null; then
    log_error "Failed to extract tarball: $tarball_path"
    rm -rf "$test_dir"
    return 1
  fi

  local binary="${test_dir}/ssbd"

  # Verify binary exists
  if [[ ! -f "$binary" ]]; then
    log_error "Binary not found in tarball: $tarball_path"
    rm -rf "$test_dir"
    return 1
  fi

  # Make executable
  chmod +x "$binary"

  # Run test: ssbd -h should return 0
  if ! "$binary" -h &>/dev/null; then
    log_error "Binary test failed: $binary -h returned non-zero"
    rm -rf "$test_dir"
    return 1
  fi

  # Verify help output contains expected text
  local help_output=$("$binary" -h 2>&1 | head -1)
  if [[ -z "$help_output" ]]; then
    log_error "Binary produced no output"
    rm -rf "$test_dir"
    return 1
  fi

  # Clean up
  rm -rf "$test_dir"

  log_success "$description test passed"
  return 0
}

# Test all built binaries
test_all_binaries() {
  log_section "Testing All Binaries"

  local test_failed=false

  # Test Arch binary (Halloween)
  local arch_tarball="${DIST_OUTPUT}/spooky_scoreboard_daemon/dist/arch/ssbd-${VERSION}-arch-x86_64.tar.gz"
  if [[ -f "$arch_tarball" ]]; then
    if ! test_binary "$arch_tarball" "Arch binary (Halloween)"; then
      test_failed=true
    fi
  else
    log_warning "Arch tarball not found, skipping test"
  fi

  echo ""

  # Test Debian binary (TCM & Evil Dead)
  local debian_tarball="${DIST_OUTPUT}/spooky_scoreboard_daemon/dist/debian/ssbd-${VERSION}-debian-x86_64.tar.gz"
  if [[ -f "$debian_tarball" ]]; then
    if ! test_binary "$debian_tarball" "Debian binary (TCM & Evil Dead)"; then
      test_failed=true
    fi
  else
    log_warning "Debian tarball not found, skipping test"
  fi

  echo ""

  if [[ "$test_failed" == true ]]; then
    log_error "Some tests failed"
    return 1
  else
    log_success "All tests passed"
    return 0
  fi
}

# ============================================================================
# Cleanup Functions
# ============================================================================

# Clean up temporary files
cleanup_temp() {
  if [[ -d "$TEMP_DIR" ]]; then
    log_info "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"
  fi
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
  log_section "Spooky Scoreboard Daemon - Master Build"

  # Parse command line arguments
  parse_arguments "$@"

  # Validation phase
  validate_version_file
  validate_docker
  validate_build_script

  # Setup phase
  clean_output
  create_output_structure

  # Build phase
  build_all_binaries

  # Package phase
  copy_installer
  create_readme

  # Test phase (optional)
  if [[ "$FLAG_TEST" == true ]]; then
    if ! test_all_binaries; then
      cleanup_temp
      exit 1
    fi
  fi

  # Cleanup
  cleanup_temp

  # Summary
  log_section "Build Complete"
  log_success "Distribution package ready at: $DIST_OUTPUT"
  echo ""
  log_info "Next steps:"
  log_info "  1. Review the contents of .build/dist/"
  log_info "  2. Use these files to create a bootable USB installer"
  log_info "  3. See INSTALL.md for deployment instructions"
  echo ""
}

# Trap errors and cleanup
trap cleanup_temp EXIT

# Run main function with all arguments
main "$@"
