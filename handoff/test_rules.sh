#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_tmp=$(mktemp -d "${TMPDIR:-/tmp}/doe-rules.XXXXXX")
trap 'rm -rf "$test_tmp"' EXIT
for game in tug snake tennis sausage; do
  "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I. "tests/test_$game.c" "core/games/$game.c" -o "$test_tmp/test_$game"
  "$test_tmp/test_$game"
done
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I. tests/test_input.c input/input.c -o "$test_tmp/test_input"
"$test_tmp/test_input"
