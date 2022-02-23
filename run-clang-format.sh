#!/usr/bin/env bash
# ---------------------------------------------------------------------------- #

set -o errexit -o pipefail -o nounset

cd "$( realpath "$0" | xargs dirname )" &&
find . -name '*.[ch]' -o -name '*.[ch]pp' | xargs clang-format -i

# ---------------------------------------------------------------------------- #
