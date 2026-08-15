#!/usr/bin/env bash
# Build installable artifacts from this tree: a Debian package and/or an AppImage.
# The .deb is the native Deepin/DTK path. The AppImage bundles Qt/DTK libraries
# but still expects host NetworkManager, ffmpeg or gst-launch, and pactl.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

APP_NAME="deepin-miracast"
VERSION="$(sed -n 's/^project(deepin-miracast VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
ARCH="$(uname -m)"
JOBS="$(nproc 2>/dev/null || echo 4)"
OUT_DIR="${ROOT}/dist"
BUILD_DEB=0
BUILD_APPIMAGE=0
DO_CLEAN=0
SKIP_BUILD=0

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

  --deb           Build a .deb with dpkg-buildpackage
  --appimage      Build an AppImage (downloads linuxdeploy if needed)
  --all           Build both (default if no target is given)
  --output DIR    Write artifacts here (default: dist/)
  --jobs N        Parallel build jobs (default: ${JOBS})
  --skip-build    Reuse the existing CMake build for the AppImage
  --clean         Remove the packaging build directory first
  -h, --help      Show this help

Examples:
  ./scripts/package.sh
  ./scripts/package.sh --deb
  ./scripts/package.sh --appimage --output ./dist
EOF
}

log() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "missing command: $1"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --deb) BUILD_DEB=1 ;;
        --appimage) BUILD_APPIMAGE=1 ;;
        --all) BUILD_DEB=1; BUILD_APPIMAGE=1 ;;
        --output) OUT_DIR="$(mkdir -p "$2" && cd "$2" && pwd)"; shift ;;
        --jobs) JOBS="$2"; shift ;;
        --skip-build) SKIP_BUILD=1 ;;
        --clean) DO_CLEAN=1 ;;
        -h|--help) usage; exit 0 ;;
        *) die "unknown option: $1" ;;
    esac
    shift
done

if [[ "${BUILD_DEB}" -eq 0 && "${BUILD_APPIMAGE}" -eq 0 ]]; then
    BUILD_DEB=1
    BUILD_APPIMAGE=1
fi

[[ -n "${VERSION}" ]] || die "could not read the project version from CMakeLists.txt"
mkdir -p "${OUT_DIR}"

PKG_BUILD="${ROOT}/build-package"
CACHE_DIR="${XDG_CACHE_HOME:-${HOME}/.cache}/${APP_NAME}/tools"

fetch_tool() {
    local url="$1" dest="$2"
    if [[ -x "${dest}" ]]; then
        return 0
    fi
    mkdir -p "$(dirname "${dest}")"
    need_cmd curl
    log "Downloading $(basename "${dest}")…"
    curl -fL --retry 3 --retry-delay 2 -o "${dest}.partial" "${url}"
    chmod +x "${dest}.partial"
    mv "${dest}.partial" "${dest}"
}

run_appimage() {
    local tool="$1"
    shift
    if [[ -e /dev/fuse ]] && [[ -r /dev/fuse ]]; then
        "${tool}" "$@"
    else
        "${tool}" --appimage-extract-and-run "$@"
    fi
}

copy_qt_plugin() {
    local src="$1" dest_dir="$2"
    [[ -e "${src}" ]] || return 0
    mkdir -p "${dest_dir}"
    cp -a "${src}" "${dest_dir}/"
}

configure_and_build() {
    local build_dir="$1"
    if [[ "${DO_CLEAN}" -eq 1 && -d "${build_dir}" ]]; then
        rm -rf "${build_dir}"
    fi
    cmake -S "${ROOT}" -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_APP=ON \
        -DBUILD_CHECKS=OFF
    cmake --build "${build_dir}" -j"${JOBS}"
}

build_deb() {
    need_cmd dpkg-buildpackage
    need_cmd dpkg-checkbuilddeps
    if ! dpkg-checkbuilddeps >/dev/null 2>&1; then
        log "Missing Debian build dependencies. On Deepin / a DTK6 system:"
        log "  sudo apt build-dep ."
        dpkg-checkbuilddeps || true
        die "install the build dependencies and re-run --deb"
    fi

    log "Building .deb ${APP_NAME}_${VERSION}…"
    dpkg-buildpackage -us -uc -b -j"${JOBS}"

    local parent
    parent="$(cd "${ROOT}/.." && pwd)"
    local copied=0
    local f
    for f in "${parent}/${APP_NAME}_${VERSION}"_*.deb \
             "${parent}/${APP_NAME}_${VERSION}"_*.buildinfo \
             "${parent}/${APP_NAME}_${VERSION}"_*.changes; do
        [[ -f "${f}" ]] || continue
        cp -a "${f}" "${OUT_DIR}/"
        copied=1
    done
    [[ "${copied}" -eq 1 ]] || die "dpkg-buildpackage finished but no .deb was found in ${parent}"
    log "Debian package written to ${OUT_DIR}/"
}

build_appimage() {
    need_cmd cmake
    need_cmd qmake6

    local linuxdeploy="${CACHE_DIR}/linuxdeploy-${ARCH}.AppImage"
    local qt_plugin="${CACHE_DIR}/linuxdeploy-plugin-qt-${ARCH}.AppImage"
    fetch_tool \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" \
        "${linuxdeploy}"
    fetch_tool \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage" \
        "${qt_plugin}"

    if [[ "${SKIP_BUILD}" -eq 0 || ! -x "${PKG_BUILD}/${APP_NAME}" ]]; then
        configure_and_build "${PKG_BUILD}"
    fi
    [[ -x "${PKG_BUILD}/${APP_NAME}" ]] || die "no ${APP_NAME} binary in ${PKG_BUILD}"

    local appdir="${OUT_DIR}/AppDir"
    rm -rf "${appdir}"
    DESTDIR="${appdir}" cmake --install "${PKG_BUILD}" --prefix /usr

    [[ -x "${appdir}/usr/bin/${APP_NAME}" ]] || die "cmake --install did not place usr/bin/${APP_NAME}"
    [[ -f "${appdir}/usr/share/applications/org.deepin.miracast.desktop" ]] \
        || die "desktop file missing from AppDir"
    [[ -f "${appdir}/usr/share/icons/hicolor/scalable/apps/${APP_NAME}.svg" ]] \
        || die "icon missing from AppDir"

    local qt_plugins
    qt_plugins="$(qmake6 -query QT_INSTALL_PLUGINS)"
    copy_qt_plugin "${qt_plugins}/styles/libchameleon.so" "${appdir}/usr/plugins/styles"
    copy_qt_plugin "${qt_plugins}/iconengines/libdicon.so" "${appdir}/usr/plugins/iconengines"
    copy_qt_plugin "${qt_plugins}/iconengines/libdsvgicon.so" "${appdir}/usr/plugins/iconengines"
    copy_qt_plugin "${qt_plugins}/platformthemes/libqdeepin.so" "${appdir}/usr/plugins/platformthemes"

    local output="${OUT_DIR}/${APP_NAME}-${VERSION}-${ARCH}.AppImage"
    rm -f "${output}"

    ln -sfn "$(basename "${qt_plugin}")" "${CACHE_DIR}/linuxdeploy-plugin-qt"

    log "Assembling AppImage…"
    (
        cd "${OUT_DIR}"
        export PATH="${CACHE_DIR}:${PATH}"
        export QMAKE="$(command -v qmake6)"
        export LINUXDEPLOY_OUTPUT_VERSION="${VERSION}"
        export OUTPUT="$(basename "${output}")"
        export EXTRA_QT_PLUGINS="styles;iconengines;platformthemes"
        run_appimage "${linuxdeploy}" \
            --appdir "${appdir}" \
            --executable "${appdir}/usr/bin/${APP_NAME}" \
            --desktop-file "${appdir}/usr/share/applications/org.deepin.miracast.desktop" \
            --icon-file "${appdir}/usr/share/icons/hicolor/scalable/apps/${APP_NAME}.svg" \
            --plugin qt \
            --output appimage
    )
    [[ -f "${output}" ]] || die "linuxdeploy did not produce ${output}"
    chmod +x "${output}"
    log "AppImage written to ${output}"
    log "Host still needs NetworkManager, ffmpeg or gst-launch-1.0, and pactl."
}

if [[ "${BUILD_DEB}" -eq 1 ]]; then
    build_deb
fi
if [[ "${BUILD_APPIMAGE}" -eq 1 ]]; then
    build_appimage
fi

log "Done. Artifacts in ${OUT_DIR}:"
ls -lh "${OUT_DIR}"/*."${ARCH}".AppImage "${OUT_DIR}"/*.deb 2>/dev/null || ls -lh "${OUT_DIR}"
