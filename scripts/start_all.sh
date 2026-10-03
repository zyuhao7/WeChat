#!/usr/bin/env bash
# Start all WeChat backend services.
#
# Order matters: VerifyServer (code mail) -> StatusServer (token/least-loaded
# pick) -> ChatServer instances -> GateServer (HTTP entry). Extra ChatServer
# instances are auto-detected from run/chatserver*/config.ini; each instance
# reads config.ini from its own working directory.
#
# Logs and PIDs are written to logs/. Pass --client to also launch the Qt GUI.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
LOGDIR="${LOGDIR:-$ROOT/logs}"
mkdir -p "$LOGDIR"

if [ ! -x "$BUILD/GateServer" ]; then
  echo "build/ not ready; run: cmake -B build && cmake --build build -j4" >&2
  exit 1
fi

start() {
  local name="$1" dir="$2"; shift 2
  if [ -f "$LOGDIR/$name.pid" ] && kill -0 "$(cat "$LOGDIR/$name.pid")" 2>/dev/null; then
    echo "$name already running (pid $(cat "$LOGDIR/$name.pid"))"
    return
  fi
  ( cd "$dir" && exec nohup "$@" ) >"$LOGDIR/$name.log" 2>&1 &
  local pid=$!
  echo "$pid" >"$LOGDIR/$name.pid"
  echo "starting $name (cwd=$dir, pid=$pid)"
}

start verify "$ROOT/VerifyServer" node server.js
start status "$ROOT/StatusServer" "$BUILD/StatusServer"
start chat_chatserver1 "$ROOT/ChatServer" "$BUILD/ChatServer"

for d in "$ROOT"/run/chatserver*/; do
  [ -f "$d/config.ini" ] || continue
  start "chat_$(basename "$d")" "$d" "$BUILD/ChatServer"
done

start gate "$ROOT/GateServer" "$BUILD/GateServer"

if [ "${1:-}" = "--client" ]; then
  ( exec "$ROOT/scripts/start_client.sh" ) >"$LOGDIR/client.log" 2>&1 &
  cpid=$!
  echo "$cpid" >"$LOGDIR/client.pid"
  echo "starting client (pid=$cpid)"
fi

echo "all started; logs in $LOGDIR"
