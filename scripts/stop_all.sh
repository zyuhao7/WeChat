#!/usr/bin/env bash
# Stop services started by start_all.sh (reads PIDs from logs/).
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOGDIR="${LOGDIR:-$ROOT/logs}"

shopt -s nullglob
for pidfile in "$LOGDIR"/*.pid; do
  name="$(basename "$pidfile" .pid)"
  pid="$(cat "$pidfile")"
  if kill -0 "$pid" 2>/dev/null; then
    kill "$pid" && echo "stopped $name (pid $pid)"
    # wait for it to actually exit so a following start_all does not race the
    # still-closing sockets (and, for redis, a half-dead PING)
    for _ in $(seq 1 50); do
      kill -0 "$pid" 2>/dev/null || break
      sleep 0.1
    done
  else
    echo "$name not running"
  fi
  rm -f "$pidfile"
done
