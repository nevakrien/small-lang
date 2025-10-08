#!/usr/bin/env bash
set -u  # undefined variable error
set -o pipefail  # catch pipeline errors

# --- Arguments ---
EXAMPLES_DIR="${1:-$(dirname "$0")/examples}"   # allow overriding the directory
SMALL_BIN="$(dirname "$0")/build/small"

if [[ ! -x "$SMALL_BIN" ]]; then
    echo "Error: $SMALL_BIN not found or not executable."
    echo "Please build the project first:"
    echo "  cmake . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build"
    exit 1
fi

if [[ ! -d "$EXAMPLES_DIR" ]]; then
    echo "Error: example directory not found: $EXAMPLES_DIR"
    exit 1
fi

echo "=== Running all .small examples in: $EXAMPLES_DIR ==="
echo

failures=0
failed_files=()

# --- Loop over all .small files ---
for file in "$EXAMPLES_DIR"/*.small; do
    [[ -e "$file" ]] || continue
    echo ">>> Running: $(basename "$file")"
    if "$SMALL_BIN" "$file"; then
        echo "✓ Success"
    else
        echo "✗ Failed: $file"
        ((failures++))
        failed_files+=("$file")
    fi
    echo
done

# --- Report summary ---
if (( failures > 0 )); then
    echo "=== $failures example(s) failed ==="
    printf ' - %s\n' "${failed_files[@]}"
    exit 1
else
    echo "=== All examples passed successfully ==="
fi
