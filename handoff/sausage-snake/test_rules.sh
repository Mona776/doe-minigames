#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/src"
test_bin=$(mktemp "${TMPDIR:-/tmp}/sausage-rules.XXXXXX")
trap 'rm -f "$test_bin"' EXIT
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I. tests/test_sausage.c core/games/sausage.c -o "$test_bin"
"$test_bin"
