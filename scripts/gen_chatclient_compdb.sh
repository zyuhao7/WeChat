#!/usr/bin/env bash
# Regenerate ChatClient/compile_commands.json.
#
# ChatClient is built with qmake, which does not emit a compilation database,
# so clangd cannot resolve the Qt headers (every <QWidget> shows as undefined).
# This derives the flags from the qmake-generated Makefile and writes a CDB the
# editor's clangd picks up via ChatClient/.clangd.
#
# Re-run after adding/removing sources or switching Qt versions.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/ChatClient"
BUILD="${BUILD:-$SRC/build}"

command -v qmake6 >/dev/null || { echo "qmake6 not found" >&2; exit 1; }
[ -f "$BUILD/Makefile" ] || {
  echo "no $BUILD/Makefile; run: cd ChatClient/build && qmake6 ../Chat.pro && make" >&2
  exit 1
}

QT_INC="$(qmake6 -query QT_INSTALL_HEADERS)"
QT_MKSPECS="$(qmake6 -query QT_INSTALL_ARCHDATA)/mkspecs/linux-g++"

DEFINES=(-DQT_DEPRECATED_WARNINGS -DQT_NO_DEBUG -DQT_WIDGETS_LIB
         -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB)
BASE=(-pipe -O2 -Wall -Wextra -fPIC -D_REENTRANT -std=gnu++17)
INCS=(-I"$SRC" -I"$BUILD"
      -I"$QT_INC" -I"$QT_INC/QtWidgets" -I"$QT_INC/QtGui"
      -I"$QT_INC/QtNetwork" -I"$QT_INC/QtCore" -I"$QT_MKSPECS")

json_args() { local out="["; local a; for a in "$@"; do out+="\"$a\","; done; printf '%s]' "${out%,}"; }

OUT="$SRC/compile_commands.json"
{
  printf '[\n'
  first=1
  for cpp in "$SRC"/*.cpp; do
    [ "$first" = 1 ] || printf ',\n'
    first=0
    obj="$BUILD/$(basename "${cpp%.cpp}").o"
    printf ' {"directory": "%s", "file": "%s", "arguments": %s}' \
      "$BUILD" "$cpp" "$(json_args g++ "${BASE[@]}" "${DEFINES[@]}" "${INCS[@]}" -c "$cpp" -o "$obj")"
  done
  printf '\n]\n'
} > "$OUT"

echo "wrote $OUT"
