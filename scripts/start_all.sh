#!/usr/bin/env bash
# Start all WeChat backend services.
#
# Order matters: Redis (state store) -> VerifyServer (code mail) ->
# StatusServer (token/least-loaded pick) -> ChatServer instances -> GateServer
# (HTTP entry). Extra ChatServer instances are auto-detected from
# run/chatserver*/config.ini; each instance reads config.ini from its own
# working directory.
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

# Redis is a hard dependency: ChatServer and VerifyServer both connect on :6380.
# Start a local instance if nothing is already answering, so the stack comes up
# in one command. Existing instances (e.g. system redis) are left untouched.
REDIS_PORT=6380
REDIS_PASS=123456
redis_up() {
  redis-cli -p "$REDIS_PORT" -a "$REDIS_PASS" --no-auth-warning PING 2>&1 | grep -q PONG
}
if redis_up; then
  echo "redis already up on :$REDIS_PORT"
elif command -v redis-server >/dev/null 2>&1; then
  start redis "$ROOT" redis-server --port "$REDIS_PORT" --requirepass "$REDIS_PASS" \
    --save "" --appendonly no --dir "$LOGDIR"
else
  echo "redis-server not found; start Redis on :$REDIS_PORT yourself" >&2
fi

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
