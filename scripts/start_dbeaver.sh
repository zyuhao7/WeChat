#!/usr/bin/env bash
# Launch the DBeaver CE SQL client on WSLg, installing it first if missing.
#
# WSLg constraint (same as the Qt client): SWT must talk X11 (not Wayland) and
# the EGL path is broken without /dev/dri, so force XWayland + software GL.
#
# MySQL to connect to (see ChatServer/config.ini): 127.0.0.1:3306, db01.
set -euo pipefail

DEB_ASSET=""
case "$(uname -m)" in
  x86_64)        DEB_ASSET="linux-x86_64.deb" ;;
  aarch64|arm64) DEB_ASSET="linux-aarch64.deb" ;;
  *) echo "unsupported arch $(uname -m); install dbeaver manually" >&2; exit 1 ;;
esac

install_dbeaver() {
  command -v apt-get >/dev/null 2>&1 || {
    echo "apt-get not found; install dbeaver-ce from https://dbeaver.io/download/" >&2
    exit 1
  }
  echo "dbeaver not found; downloading the latest DBeaver CE .deb ..." >&2

  local api="https://api.github.com/repos/dbeaver/dbeaver/releases/latest"
  local url
  url="$(curl -fsSL "$api" \
          | grep -o "\"browser_download_url\": *\"[^\"]*${DEB_ASSET}\"" \
          | sed 's/.*"\(https[^"]*\)"/\1/' | head -n1 || true)"
  [ -n "$url" ] || { echo "could not resolve a ${DEB_ASSET} download URL" >&2; exit 1; }

  local deb
  deb="$(mktemp -d)/dbeaver-ce.deb"
  echo "downloading $url" >&2
  curl -fL --progress-bar "$url" -o "$deb"

  # apt resolves the .deb's dependencies (default-jre, etc.) from the local file.
  sudo apt-get install -y "$deb"
  rm -rf "$(dirname "$deb")"
}

command -v dbeaver >/dev/null 2>&1 || install_dbeaver

export DISPLAY="${DISPLAY:-:0}"
export GDK_BACKEND="${GDK_BACKEND:-x11}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"

exec dbeaver "$@"
