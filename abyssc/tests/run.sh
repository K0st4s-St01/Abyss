#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER="$ROOT/build/abyssc"
TMPDIR=${TMPDIR:-/tmp}/abyssc_tests_$$

mkdir -p "$TMPDIR"
trap 'rm -rf "$TMPDIR"' EXIT

run_case() {
    src=$1
    expected=$2
    out="$TMPDIR/$(basename "$src" .as)"

    "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL $src: expected $expected, got $actual" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_case high_priority_literals.as 42
run_case switch.as 90
run_case pointer_array.as 60
