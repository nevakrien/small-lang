#!/usr/bin/env bash
set -euo pipefail

EXAMPLES_DIR="$(dirname "$0")/examples"
SMALL_BIN="$(dirname "$0")/build/small"

if [[ ! -x "$SMALL_BIN" ]]; then
    echo "Error: $SMALL_BIN not found or not executable."
    echo "Please build the project first:"
    echo "  cmake . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build"
    exit 1
fi

echo "=== Running all .small examples ==="
echo

# Track failures
failures=0

for file in "$EXAMPLES_DIR"/*.small; do
    [[ -e "$file" ]] || continue
    echo ">>> Running: $(basename "$file")"
    if "$SMALL_BIN" "$file"; then
        echo "✓ Success"
    else
        echo "✗ Failed: $file"
        ((failures++))
    fi
    echo
done

if (( failures > 0 )); then
    echo "=== $failures example(s) failed ==="
    exit 1
else
    echo "=== All examples passed successfully ==="
fi
