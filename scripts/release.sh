#!/usr/bin/env bash
set -uo pipefail
# ==============================================================
# Release packaging script for N_m3u8DL-RE GUI (Qt)
# Builds Release, deploys Qt DLLs, copies resources, creates zip.
# Usage: ./scripts/release.sh [version]
# ==============================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

QT_VER="6.11.1"
QT_KIT="llvm-mingw_64"
QT_DIR="/d/Qt/${QT_VER}/${QT_KIT}"
TOOLCHAIN="/d/Qt/Tools/llvm-mingw1706_64"

VERSION="${1:-1.0.0}"
RELEASE_NAME="N_m3u8DL-RE-GUI-Qt-v${VERSION}"
RELEASE_DIR="$PROJECT_DIR/release/$RELEASE_NAME"

echo "=========================================="
echo " N_m3u8DL-RE GUI Qt  -  Release Builder"
echo " Version: ${VERSION}"
echo "=========================================="

# ---- Step 1: Build ----
echo ""
echo "[1/4] Building Release..."
mkdir -p "$BUILD_DIR"

export PATH="$TOOLCHAIN/bin:$QT_DIR/bin:$PATH"

cmake -G "MinGW Makefiles" \
    -S "$PROJECT_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_C_COMPILER="$TOOLCHAIN/bin/x86_64-w64-mingw32-gcc.exe" \
    -DCMAKE_CXX_COMPILER="$TOOLCHAIN/bin/x86_64-w64-mingw32-g++.exe" \
    -DCMAKE_RC_COMPILER="$TOOLCHAIN/bin/x86_64-w64-mingw32-windres.exe"

cmake --build "$BUILD_DIR" --config Release
echo "Build done."

# ---- Step 2: Deploy Qt DLLs ----
echo ""
echo "[2/4] Deploying Qt dependencies..."
"$QT_DIR/bin/windeployqt" --release --no-translations --no-compiler-runtime \
    "$BUILD_DIR/N_m3u8DL_RE_GUI_Qt.exe" 2>&1 || echo "(windeployqt warning - may be false alarm, continuing)"
echo "Deploy done."

# ---- Step 3: Create release directory ----
echo ""
echo "[3/4] Assembling release package..."
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

cp "$BUILD_DIR/N_m3u8DL_RE_GUI_Qt.exe" "$RELEASE_DIR/"
cp "$BUILD_DIR"/*.dll "$RELEASE_DIR/" 2>/dev/null || true

for dir in platforms styles imageformats iconengines tls generic networkinformation translations; do
    [ -d "$BUILD_DIR/$dir" ] && cp -r "$BUILD_DIR/$dir" "$RELEASE_DIR/$dir"
done

[ -d "$PROJECT_DIR/third_party" ] && cp -r "$PROJECT_DIR/third_party" "$RELEASE_DIR/"
[ -f "$PROJECT_DIR/resources/app_icon.png" ] && cp "$PROJECT_DIR/resources/app_icon.png" "$RELEASE_DIR/"
[ -f "$PROJECT_DIR/README.md" ] && cp "$PROJECT_DIR/README.md" "$RELEASE_DIR/"
[ -f "$PROJECT_DIR/LICENSE" ] && cp "$PROJECT_DIR/LICENSE" "$RELEASE_DIR/"

if [ -f "$PROJECT_DIR/doc/N_m3u8DL-RE_README.md" ]; then
    mkdir -p "$RELEASE_DIR/doc"
    cp "$PROJECT_DIR/doc/N_m3u8DL-RE_README.md" "$RELEASE_DIR/doc/"
fi

echo "Release assembled in: $RELEASE_DIR"

# ---- Step 4: Package ----
echo ""
echo "[4/4] Creating archive..."
cd "$PROJECT_DIR/release"

if command -v zip &>/dev/null; then
    rm -f "${RELEASE_NAME}.zip"
    zip -r "${RELEASE_NAME}.zip" "$RELEASE_NAME"
    echo "Archive: ${PROJECT_DIR}/release/${RELEASE_NAME}.zip"
elif command -v powershell &>/dev/null; then
    powershell -NoProfile -Command \
        "Compress-Archive -Path '${RELEASE_NAME}' -DestinationPath '${RELEASE_NAME}.zip' -Force"
    echo "Archive: ${PROJECT_DIR}/release/${RELEASE_NAME}.zip"
else
    echo "No zip tool found - release folder is ready at:"
    echo "  $RELEASE_DIR"
fi

cd "$PROJECT_DIR"

echo ""
echo "=========================================="
echo " Release ready:"
echo "   Folder: $RELEASE_DIR"
echo "=========================================="
