#!/usr/bin/env bash
# Optional headless renderer/integration tooling for this Debian cloud machine.
set -euo pipefail
if [[ $(uname -s) != Linux || $(uname -m) != x86_64 ]]; then
    echo 'This helper targets the Linux x86_64 cloud machine.' >&2
    exit 1
fi

tools_root=/workspace/.tools
source_root="$tools_root/ppsspp-source"
build_root="$tools_root/ppsspp-build"
deps_root="$tools_root/host-deps"
apt_root="$tools_root/apt"
revision=8c0c848a38b91da4a80ed7f29e2196d6538cde15
mkdir -p "$tools_root/downloads" "$deps_root" "$build_root"
if [[ ! -x "$tools_root/host-build-venv/bin/cmake" ]]; then
    python3 -m venv "$tools_root/host-build-venv"
    PIP_CACHE_DIR="$tools_root/pip-cache" "$tools_root/host-build-venv/bin/python" \
        -m pip install cmake==4.1.2
fi
cmake="$tools_root/host-build-venv/bin/cmake"

if [[ ! -d "$source_root" ]]; then
    git clone --no-checkout --depth 1 https://github.com/hrydgard/ppsspp.git "$source_root"
    git -C "$source_root" fetch --depth 1 origin "$revision"
    git -C "$source_root" checkout --detach "$revision"
fi
if [[ $(git -C "$source_root" rev-parse HEAD) != "$revision" ]]; then
    echo 'The existing PPSSPP checkout has a different revision; preserve it before replacing it.' >&2
    exit 1
fi
git -C "$source_root" diff --quiet --ignore-submodules=all
git -C "$source_root" diff --cached --quiet --ignore-submodules=all
git -C "$source_root" submodule update --init --recursive --depth 1 --jobs 6 \
    ext/armips ext/cpu_features ext/glslang ext/freetype ext/SPIRV-Cross \
    ext/rapidjson ext/libchdr ext/rcheevos ext/zstd ext/nanosvg ext/lua \
    ext/aemu_postoffice ext/OpenXR-SDK ext/miniupnp

# Keep apt state and extracted headers local. Signed Debian repository metadata
# and apt's package checksums remain enabled; no system package install occurs.
mkdir -p "$apt_root/state/lists/partial" "$apt_root/cache/archives/partial"
python3 - "$apt_root" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
(root / 'sources.list').write_text(
    'deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] '
    'https://deb.debian.org/debian trixie main\n')
(root / 'apt.conf').write_text(f'''Dir::Etc::sourcelist "{root}/sources.list";
Dir::Etc::sourceparts "-";
Dir::State "{root}/state";
Dir::State::status "/var/lib/dpkg/status";
Dir::Cache "{root}/cache";
Dir::Log "{root}/log";
APT::Get::List-Cleanup "false";
#clear APT::Update::Post-Invoke;
#clear APT::Update::Post-Invoke-Success;
''')
PY
apt-get -c "$apt_root/apt.conf" update
deb_staging=$(mktemp -d "$tools_root/downloads/ppsspp-debs.XXXXXX")
trap 'rm -rf "$deb_staging"' EXIT
(
    cd "$deb_staging"
    apt-get -c "$apt_root/apt.conf" download libgl-dev libglx-dev libopengl-dev \
        libglu1-mesa-dev libx11-dev x11proto-dev libxau-dev libxdmcp-dev xtrans-dev
    for archive in ./*.deb; do dpkg-deb -x "$archive" "$deps_root"; done
)
rm -rf "$deb_staging"
trap - EXIT

"$cmake" -S "$source_root" -B "$build_root" -DCMAKE_BUILD_TYPE=Release \
    -DHEADLESS=ON -DHEADLESS_CROSS=ON -DUSING_X11_VULKAN=OFF -DUSE_WAYLAND_WSI=OFF \
    -DUSE_FFMPEG=OFF -DUSE_DISCORD=OFF -DUSE_MINIUPNPC=OFF \
    -DUSE_SYSTEM_FREETYPE=OFF -DUSE_SYSTEM_LIBPNG=OFF -DUSE_SYSTEM_LIBZIP=OFF \
    -DUSE_SYSTEM_ZSTD=OFF -DATLAS_TOOL=OFF -DUNITTEST=OFF -DNO_OPENGL=OFF \
    -DOPENGL_INCLUDE_DIR="$deps_root/usr/include" \
    -DOPENGL_gl_LIBRARY=/usr/lib/x86_64-linux-gnu/libGL.so.1 \
    -DOPENGL_glx_LIBRARY=/usr/lib/x86_64-linux-gnu/libGLX.so.0 \
    -DOPENGL_opengl_LIBRARY=/usr/lib/x86_64-linux-gnu/libOpenGL.so.0 \
    -DCMAKE_C_FLAGS="-I$deps_root/usr/include" \
    -DCMAKE_CXX_FLAGS="-I$deps_root/usr/include" \
    -DCMAKE_CXX_STANDARD_LIBRARIES='/usr/lib/x86_64-linux-gnu/libGL.so.1 /usr/lib/x86_64-linux-gnu/libGLX.so.0 /usr/lib/x86_64-linux-gnu/libOpenGL.so.0' \
    > "$build_root/configure.log" 2>&1 || { tail -n 60 "$build_root/configure.log"; exit 1; }
"$cmake" --build "$build_root" --target PPSSPPHeadless --parallel 8 \
    > "$build_root/build.log" 2>&1 || { tail -n 60 "$build_root/build.log"; exit 1; }
test -x "$build_root/PPSSPPHeadless"
echo "Headless PPSSPP $revision prepared at $build_root/PPSSPPHeadless"
