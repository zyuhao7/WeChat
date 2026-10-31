#!/usr/bin/env bash
# Shared fcitx5 bootstrap for the WSLg launchers. Source this file, do not run it.
#
# WSLg ships no input method, and fcitx5 talks to its clients over the session
# bus. WSLg can leave a stale /run/user/<uid>/bus socket with no daemon behind
# it: fcitx5 then starts but every client stays unreachable, so Ctrl+Space
# cannot enable Chinese input. wayland/waylandim stay disabled because WSLg
# denies the input-method protocol and the fatal error kills the daemon.
ensure_ime() {
  command -v fcitx5 >/dev/null 2>&1 || return 0

  if ! dbus-send --session --print-reply --dest=org.freedesktop.DBus / \
      org.freedesktop.DBus.GetId >/dev/null 2>&1; then
    local runtime="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
    export DBUS_SESSION_BUS_ADDRESS="unix:path=$runtime/bus"
    mkdir -p "$runtime" 2>/dev/null || true
    # nothing answers on the old socket, so removing it cannot cut off a live daemon
    [ -S "$runtime/bus" ] && rm -f "$runtime/bus" || true
    dbus-daemon --session --fork --address="$DBUS_SESSION_BUS_ADDRESS" >/dev/null 2>&1 || true
  fi

  # a daemon attached to a bus that has since died cannot be reached either
  if ! fcitx5-remote -n >/dev/null 2>&1; then
    pkill -x fcitx5 >/dev/null 2>&1 || true
    fcitx5 -d --disable wayland,waylandim >/dev/null 2>&1 || true
  fi

  export QT_IM_MODULE="${QT_IM_MODULE:-fcitx}"
  export GTK_IM_MODULE="${GTK_IM_MODULE:-fcitx}"
  export XMODIFIERS="${XMODIFIERS:-@im=fcitx}"
  export SDL_IM_MODULE="${SDL_IM_MODULE:-fcitx}"
}
