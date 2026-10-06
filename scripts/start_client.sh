#!/usr/bin/env bash
# Launch the Qt ChatClient (Qt6). On WSLg it auto-fixes XDG_RUNTIME_DIR so the
# app can reach the Wayland/X11 socket.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/ChatClient/build/bin"

if [ ! -x "$BIN/Chat" ]; then
  echo "client not built; run:" >&2
  echo "  cd $ROOT/ChatClient && mkdir -p build && cd build && qmake6 ../Chat.pro && make -j4" >&2
  exit 1
fi

# The app reads config.ini and static/ from its working directory.
[ -f "$BIN/config.ini" ] || cp "$ROOT/ChatClient/config.ini" "$BIN/"
[ -d "$BIN/static" ] || cp -r "$ROOT/ChatClient/static" "$BIN/"

# WSLg: the wayland socket lives in /mnt/wslg/runtime-dir, which is 0777 and
# makes Qt warn about XDG_RUNTIME_DIR permissions. Keep XDG_RUNTIME_DIR at the
# private 0700 dir and symlink the socket into it instead.
wslg_runtime=/mnt/wslg/runtime-dir
if [ -S "$wslg_runtime/wayland-0" ]; then
  export WAYLAND_DISPLAY=wayland-0
  runtime="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
  if [ -S "$runtime/wayland-0" ]; then
    export XDG_RUNTIME_DIR="$runtime"
  elif mkdir -p "$runtime" && chmod 700 "$runtime" \
       && ln -sf "$wslg_runtime/wayland-0" "$runtime/wayland-0" \
       && ln -sf "$wslg_runtime/wayland-0.lock" "$runtime/wayland-0.lock"; then
    export XDG_RUNTIME_DIR="$runtime"
  else
    export XDG_RUNTIME_DIR="$wslg_runtime"
  fi
fi
export DISPLAY="${DISPLAY:-:0}"

# WSLg exposes both XWayland and a Wayland socket, but its Wayland EGL path is
# broken (no /dev/dri access -> "failed to create dri2 screen"), so a Qt app
# that auto-selects the wayland platform backend never paints a window. Force
# XWayland + software GL, the path verified to work here.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"

# WSLg ships no input method, so Chinese text could never reach the Qt input
# widgets. If fcitx5 is installed, make sure its daemon is up and point the
# toolkits at it (Qt/GTK read these; XMODIFIERS covers the XIM fallback).
# wayland/waylandim are disabled: WSLg's compositor denies the input-method
# protocol and the resulting fatal Wayland error kills the whole daemon.
if command -v fcitx5 >/dev/null 2>&1; then
  pgrep -x fcitx5 >/dev/null 2>&1 || fcitx5 -d --disable wayland,waylandim >/dev/null 2>&1 || true
  export QT_IM_MODULE="${QT_IM_MODULE:-fcitx}"
  export GTK_IM_MODULE="${GTK_IM_MODULE:-fcitx}"
  export XMODIFIERS="${XMODIFIERS:-@im=fcitx}"
  export SDL_IM_MODULE="${SDL_IM_MODULE:-fcitx}"
fi

cd "$BIN"
exec ./Chat "$@"
