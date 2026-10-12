#!/usr/bin/env bash
# Format first-party C++ sources with the repo's .clang-format.
#   scripts/format.sh           rewrite files in place
#   scripts/format.sh --check   fail if any file needs formatting (used by CI)
# Pin the formatter with CLANG_FORMAT=clang-format-18 so local runs match CI.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
CLANG_FORMAT="${CLANG_FORMAT:-clang-format-18}"
command -v "$CLANG_FORMAT" >/dev/null || CLANG_FORMAT=clang-format

mapfile -t files < <(git ls-files '*.cpp' '*.h' '*.hpp' '*.cc' ':!:ChatClient/third_party/**')

if [[ "${1:-}" == "--check" ]]; then
    "$CLANG_FORMAT" --dry-run --Werror "${files[@]}"
    echo "formatting ok (${#files[@]} files)"
else
    "$CLANG_FORMAT" -i "${files[@]}"
fi
