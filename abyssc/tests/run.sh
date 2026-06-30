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

run_ir_contains() {
    src=$1
    pattern=$2
    ll="$ROOT/tests/$src.ll"

    "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -q "$pattern" "$ll"; then
        echo "FAIL $src: IR missing pattern $pattern" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_case high_priority_literals.as 42
run_case switch.as 90
run_case pointer_array.as 60
run_case else_if.as 20
run_case enum_alias.as 9
run_case interface_variadic.as 0
run_case logical_short_circuit.as 0
run_case pointer_deref.as 7
run_case struct_arrow.as 7
run_case float_numeric.as 42
run_case bool_literals.as 12
run_case for_alias_decl.as 10
run_case do_while_control.as 8
run_case casts.as 13
run_case forward_calls.as 26
run_case struct_methods.as 27
run_case increment_decrement.as 3
run_case struct_pointer_field.as 33
run_case numeric_conditions.as 21
run_case self_methods.as 47
run_case global_vars.as 28
run_case static_linkage.as 26
run_case sizeof.as 23
run_case fixed_arrays.as 44
run_case array_decay.as 51
run_case conditional_expr.as 31
run_case struct_initializers.as 36
run_ir_contains extern_globals.as "@external_counter = external global i32"
run_case comments.as 7
run_case numeric_literals.as 81
run_case char_escapes.as 150
run_case unsigned_ops.as 37
