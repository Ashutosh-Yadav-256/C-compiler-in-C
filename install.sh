#!/usr/bin/env bash
set -e

REPO="Ashutosh-Yadav-256/C-compiler-in-C"
INSTALL_DIR="/usr/local/bin"

echo "========================================="
echo " Installing C-Compiler (Native C to ASM) "
echo "========================================="

command -v gcc >/dev/null 2>&1 || { echo >&2 "Error: gcc is required but not installed. Aborting."; exit 1; }
command -v make >/dev/null 2>&1 || { echo >&2 "Error: make is required but not installed. Aborting."; exit 1; }

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT INT TERM
cd "$TMP_DIR"

echo "Cloning latest source from GitHub..."
git clone --depth 1 "https://github.com/${REPO}.git" .

echo "Building release binary..."
make release

echo "Installing binary to ${INSTALL_DIR}/ccompiler..."
if [ -w "$INSTALL_DIR" ]; then
    cp ccompiler "${INSTALL_DIR}/ccompiler"
    chmod +x "${INSTALL_DIR}/ccompiler"
else
    sudo cp ccompiler "${INSTALL_DIR}/ccompiler"
    sudo chmod +x "${INSTALL_DIR}/ccompiler"
fi

cd ~
rm -rf "$TMP_DIR"

echo ""
echo "Successfully installed ccompiler!"
echo "Usage: ccompiler <input.c> <output.asm>"
