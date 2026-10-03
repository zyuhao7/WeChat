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

# WSLg: the default XDG_RUNTIME_DIR usually lacks the wayland socket.
if [ -S /mnt/wslg/runtime-dir/wayland-0 ] && [ ! -S "${XDG_RUNTIME_DIR:-/nonexistent}/wayland-0" ]; then
  export XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir
  export WAYLAND_DISPLAY=wayland-0
fi
export DISPLAY="${DISPLAY:-:0}"

cd "$BIN"
exec ./Chat "$@"
