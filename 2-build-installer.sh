#!/usr/bin/env bash
# 2-build-installer.sh
# Creates bootable ISO installer using Linux Live Kit's EXACT process
# No manual mkisofs, no isohybrid - just use what Linux Live Kit generates

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION=$(cat "$SCRIPT_DIR/VERSION" | tr -d '[:space:]')
DIST_DIR="$SCRIPT_DIR/.build/dist"
OUTPUT_DIR="$SCRIPT_DIR/installer-output"
BUILD_DIR="$SCRIPT_DIR/.installer-build"

DOCKER_IMAGE="ssbd-installer:latest"

# Colors
log_info()    { echo -e "\033[0;34m[INFO]\033[0m $1"; }
log_success() { echo -e "\033[0;32m[✓]\033[0m $1"; }
log_error()   { echo -e "\033[0;31m[ERROR]\033[0m $1" >&2; }

echo "============================================================================"
echo "  Spooky Scoreboard Installer Builder"
echo "============================================================================"
echo ""
log_info "Version: $VERSION"
echo ""

# Check prerequisites
if [[ ! -d "$DIST_DIR" ]]; then
  log_error ".build/dist/ not found. Run ./build-binaries.sh first"
  exit 1
fi

# Clean old build
if [[ "${1:-}" == "--clean" ]]; then
  log_info "Cleaning previous build..."
  rm -rf "$BUILD_DIR" "$OUTPUT_DIR"
fi

# Create directories
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

# Build Docker image
log_info "Building Docker image..."
docker build -t "$DOCKER_IMAGE" -f installer/Dockerfile . >/dev/null

log_success "Docker image ready"
echo ""

# Build installer in Docker
log_info "Running Linux Live Kit build (this takes several minutes)..."
echo ""

docker run --rm \
  -v "$DIST_DIR:/dist:ro" \
  -v "$SCRIPT_DIR/installer:/installer-config:ro" \
  -v "$BUILD_DIR:/build-output" \
  "$DOCKER_IMAGE" \
  bash -c '
set -e

echo "============================================================================"
echo "  Linux Live Kit Build"
echo "============================================================================"
echo ""

# Configure Linux Live Kit
cd /linux-live

# Copy config
if [[ -f /installer-config/config ]]; then
  cp /installer-config/config ./config
fi

# Setup bootfiles
mkdir -p bootfiles
if [[ -f /installer-config/syslinux.cfg ]]; then
  cp /installer-config/syslinux.cfg bootfiles/syslinux.cfg
fi

# Setup rootcopy (files to include in live system)
rm -rf rootcopy
mkdir -p rootcopy/root/ssbd-installer

# Copy installer files
cp -a /dist/spooky_scoreboard_daemon rootcopy/root/ssbd-installer/
cp -a /dist/installer rootcopy/root/ssbd-installer/
cp /dist/VERSION rootcopy/root/ssbd-installer/

# Auto-login and launch installer
mkdir -p rootcopy/etc/systemd/system/getty@tty1.service.d
if [[ -f /installer-config/getty@tty1-override.conf ]]; then
  cp /installer-config/getty@tty1-override.conf rootcopy/etc/systemd/system/getty@tty1.service.d/override.conf
fi

cat > rootcopy/root/.bashrc <<'"'"'BASHRC'"'"'
# Auto-launch installer on first login
if [[ -z "$INSTALLER_LAUNCHED" ]] && [[ -f /root/ssbd-installer/installer/install.sh ]]; then
  export INSTALLER_LAUNCHED=1
  cd /root/ssbd-installer
  exec ./installer/install.sh
fi
BASHRC

echo "==> Running Linux Live Kit build script..."
echo ""

# Run Linux Live Kit build
./build

echo ""
echo "==> Linux Live Kit build complete"
echo ""

# Find the generated ZIP script (proper method for USB boot)
ZIP_SCRIPT=$(ls /tmp/gen_*_zip.sh 2>/dev/null | head -1)

if [[ -z "$ZIP_SCRIPT" ]] || [[ ! -f "$ZIP_SCRIPT" ]]; then
  echo "ERROR: Linux Live Kit did not generate ZIP script"
  ls -la /tmp/gen_*.sh 2>/dev/null || true
  exit 1
fi

echo "==> Found generated ZIP script: $ZIP_SCRIPT"
echo "==> Script contents:"
cat "$ZIP_SCRIPT"
echo ""

# Run the generated ZIP script
echo "==> Running Linux Live Kit'"'"'s ZIP generation script..."
bash "$ZIP_SCRIPT"

# Find the generated ZIP
GENERATED_ZIP=$(ls /tmp/*.zip 2>/dev/null | head -1)

if [[ -z "$GENERATED_ZIP" ]] || [[ ! -f "$GENERATED_ZIP" ]]; then
  echo "ERROR: ZIP generation script did not create ZIP"
  ls -la /tmp/*.zip 2>/dev/null || true
  exit 1
fi

echo "==> Generated ZIP: $GENERATED_ZIP"
echo "==> Size: $(du -h "$GENERATED_ZIP" | cut -f1)"

# Copy ZIP to output with versioned name
cp "$GENERATED_ZIP" /build-output/installer.zip

echo ""
echo "==> ZIP creation complete"
echo ""
echo "To install to USB:"
echo "  1. Format USB as FAT32"
echo "  2. Unzip installer.zip to USB root"
echo "  3. On Linux: cd /mnt/usb/spooky-scoreboard-installer/boot && sudo ./bootinst.sh"
echo "  4. On Windows: Run bootinst.bat from USB"
'

if [[ ! -f "$BUILD_DIR/installer.zip" ]]; then
  log_error "ZIP was not created"
  exit 1
fi

# Move to final location with version
mv "$BUILD_DIR/installer.zip" "$OUTPUT_DIR/ssbd-installer-${VERSION}.zip"

# Create checksum
cd "$OUTPUT_DIR"
sha256sum "ssbd-installer-${VERSION}.zip" > "ssbd-installer-${VERSION}.sha256"

echo ""
echo "============================================================================"
log_success "Build Complete"
echo "============================================================================"
echo ""
echo "Output:"
echo "  ZIP: $OUTPUT_DIR/ssbd-installer-${VERSION}.zip ($(du -h "$OUTPUT_DIR/ssbd-installer-${VERSION}.zip" | cut -f1))"
echo "  SHA: $OUTPUT_DIR/ssbd-installer-${VERSION}.sha256"
echo ""
echo "Install to USB:"
echo "  1. Format USB drive as FAT32"
echo "  2. Unzip ssbd-installer-${VERSION}.zip to USB root"
echo "  3. On Linux: cd /mnt/usb/spooky-scoreboard-installer/boot && sudo ./bootinst.sh"
echo "  4. On Windows: Run bootinst.bat from USB"
echo ""
