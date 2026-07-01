#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPILER="$ROOT/build/abyssc"
DWELLERPKG="$ROOT/../dwellerpkg/build/dwellerpkg"
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

run_fail() {
    src=$1

    set +e
    "$COMPILER" "$ROOT/tests/$src" >/dev/null 2>"$TMPDIR/$src.err"
    actual=$?
    set -e

    if [ "$actual" -eq 0 ]; then
        echo "FAIL $src: expected compiler failure" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_import_struct_metadata() {
    src=import_struct_metadata.as
    home="$TMPDIR/home"
    pkg="$home/.dwellerpkg/packages/geom"
    ll="$ROOT/tests/$src.ll"

    mkdir -p "$pkg"
    HOME="$home" python3 -c '
import json
import struct
from pathlib import Path

metadata = {
    "name": "geom",
    "version": "1.0.0",
    "structs": [
        {
            "name": "Point",
            "fields": [
                {"name": "x", "type": "i32", "is_ptr": 0},
                {"name": "y", "type": "i32", "is_ptr": 0},
            ],
        }
    ],
    "functions": [],
}
data = json.dumps(metadata, separators=(",", ":")).encode("utf-8")
path = Path.home() / ".dwellerpkg" / "packages" / "geom" / "geom.abyss_a"
path.write_bytes(b"DWELLERPKG\0" + struct.pack("<I", len(data)) + data)
'
    HOME="$home" "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -Fq "%Point = type { i32, i32 }" "$ll"; then
        echo "FAIL $src: imported struct metadata was not emitted" >&2
        exit 1
    fi
    echo "PASS $src"

    src=import_duplicate_metadata.as
    ll="$ROOT/tests/$src.ll"
    HOME="$home" "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -Fq "%Point = type { i32, i32 }" "$ll"; then
        echo "FAIL $src: duplicate import did not expose struct metadata" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_import_rich_metadata() {
    src=import_rich_metadata.as
    home="$TMPDIR/home_rich"
    pkg="$home/.dwellerpkg/packages/meta"
    ll="$ROOT/tests/$src.ll"

    mkdir -p "$pkg"
    HOME="$home" python3 -c '
import json
import struct
from pathlib import Path

metadata = {
    "name": "meta",
    "version": "1.0.0",
    "type_aliases": [
        {"name": "Count", "target_type": "i32"},
    ],
    "enums": [
        {"name": "Color", "variants": [{"name": "Red", "value": 7}]},
    ],
    "interfaces": [
        {
            "name": "Readable",
            "methods": [
                {"name": "read", "return_type": "i32", "params": []},
            ],
        },
    ],
    "structs": [
        {
            "name": "Point",
            "fields": [
                {"name": "x", "type": "i32", "is_ptr": 0},
                {"name": "y", "type": "i32", "is_ptr": 0},
            ],
        },
    ],
    "globals": [
        {"name": "imported_counter", "type": "i32", "is_ptr": 0, "is_extern": 1},
    ],
    "functions": [],
}
data = json.dumps(metadata, separators=(",", ":")).encode("utf-8")
path = Path.home() / ".dwellerpkg" / "packages" / "meta" / "meta.abyss_a"
path.write_bytes(b"DWELLERPKG\0" + struct.pack("<I", len(data)) + data)
'
    HOME="$home" "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -Fq "%Point = type { i32, i32 }" "$ll"; then
        echo "FAIL $src: imported struct metadata was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "@imported_counter = external global i32" "$ll"; then
        echo "FAIL $src: imported global metadata was not emitted" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_import_struct_methods_metadata() {
    src=import_struct_methods_metadata.as
    home="$TMPDIR/home_methods"
    pkg="$home/.dwellerpkg/packages/impl"
    ll="$ROOT/tests/$src.ll"

    mkdir -p "$pkg"
    HOME="$home" python3 -c '
import json
import struct
from pathlib import Path

metadata = {
    "name": "impl",
    "version": "1.0.0",
    "structs": [
        {
            "name": "Counter",
            "fields": [
                {"name": "value", "type": "i32", "is_ptr": 0},
            ],
            "methods": [
                {
                    "name": "read",
                    "return_type": "i32",
                    "params": [],
                    "is_extern": 1,
                    "is_variadic": 0,
                },
            ],
        },
    ],
    "functions": [],
}
data = json.dumps(metadata, separators=(",", ":")).encode("utf-8")
path = Path.home() / ".dwellerpkg" / "packages" / "impl" / "impl.abyss_a"
path.write_bytes(b"DWELLERPKG\0" + struct.pack("<I", len(data)) + data)
'
    HOME="$home" "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -Fq "%Counter = type { i32 }" "$ll"; then
        echo "FAIL $src: imported struct metadata was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "declare i32 @Counter.read(ptr)" "$ll"; then
        echo "FAIL $src: imported struct method declaration was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "store ptr @Counter.read" "$ll"; then
        echo "FAIL $src: imported struct method was not used in interface dispatch" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_import_namespace_metadata() {
    src=import_namespace_metadata.as
    home="$TMPDIR/home_namespace"
    ll="$ROOT/tests/$src.ll"

    mkdir -p "$home/.dwellerpkg/packages/left" "$home/.dwellerpkg/packages/right"
    HOME="$home" python3 -c '
import json
import struct
from pathlib import Path

def write_pkg(name):
    metadata = {
        "name": name,
        "version": "1.0.0",
        "structs": [
            {
                "name": "Point",
                "fields": [
                    {"name": "x", "type": "i32", "is_ptr": 0},
                ],
            },
        ],
        "functions": [
            {"name": "answer", "return_type": "i32", "params": [], "is_extern": 1},
        ],
    }
    data = json.dumps(metadata, separators=(",", ":")).encode("utf-8")
    path = Path.home() / ".dwellerpkg" / "packages" / name / f"{name}.abyss_a"
    path.write_bytes(b"DWELLERPKG\0" + struct.pack("<I", len(data)) + data)

write_pkg("left")
write_pkg("right")
'
    HOME="$home" "$COMPILER" -emit-llvm "$ROOT/tests/$src" >/dev/null
    if ! grep -Fq "%left.Point = type { i32 }" "$ll"; then
        echo "FAIL $src: qualified left struct metadata was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "%right.Point = type { i32 }" "$ll"; then
        echo "FAIL $src: qualified right struct metadata was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "declare i32 @left.answer()" "$ll"; then
        echo "FAIL $src: qualified left function metadata was not emitted" >&2
        exit 1
    fi
    if ! grep -Fq "declare i32 @right.answer()" "$ll"; then
        echo "FAIL $src: qualified right function metadata was not emitted" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_import_namespace_link() {
    src=import_namespace_link.as
    home="$TMPDIR/home_namespace_link"
    out="$TMPDIR/import_namespace_link"

    mkdir -p "$home/.dwellerpkg/packages/left" "$home/.dwellerpkg/packages/right"
    HOME="$home" COMPILER="$COMPILER" TMPROOT="$TMPDIR" python3 -c '
import os
from pathlib import Path

tmp = Path(os.environ["TMPROOT"])
sources = {
    "left": """
struct Counter {
    i32 value;

    i32 read() {
        return value + 1;
    }

    static i32 base() {
        return 3;
    }
}

i32 answer() {
    return 10;
}
""",
    "right": """
struct Counter {
    i32 value;

    i32 read() {
        return value + 2;
    }

    static i32 base() {
        return 4;
    }
}

i32 answer() {
    return 20;
}
""",
}
for name, source in sources.items():
    (tmp / f"{name}.as").write_text(source)
'
    "$COMPILER" --package-prefix left -c -o "$TMPDIR/left.o" "$TMPDIR/left.as" >/dev/null
    "$COMPILER" --package-prefix right -c -o "$TMPDIR/right.o" "$TMPDIR/right.as" >/dev/null
    ar rcs "$TMPDIR/left.raw.a" "$TMPDIR/left.o"
    ar rcs "$TMPDIR/right.raw.a" "$TMPDIR/right.o"
    HOME="$home" TMPROOT="$TMPDIR" python3 -c '
import json
import os
import struct
from pathlib import Path

home = Path.home()
tmp = Path(os.environ["TMPROOT"])

def write_pkg(name):
    metadata = {
        "name": name,
        "version": "1.0.0",
        "structs": [
            {
                "name": "Counter",
                "fields": [
                    {"name": "value", "type": "i32", "is_ptr": 0},
                ],
                "methods": [
                    {
                        "name": "read",
                        "return_type": "i32",
                        "params": [],
                        "is_extern": 1,
                        "is_static": 0,
                        "is_variadic": 0,
                    },
                    {
                        "name": "base",
                        "return_type": "i32",
                        "params": [],
                        "is_extern": 1,
                        "is_static": 1,
                        "is_variadic": 0,
                    },
                ],
            },
        ],
        "functions": [
            {"name": "answer", "return_type": "i32", "params": [], "is_extern": 1, "is_variadic": 0},
        ],
    }
    data = json.dumps(metadata, separators=(",", ":")).encode("utf-8")
    raw = (tmp / f"{name}.raw.a").read_bytes()
    path = home / ".dwellerpkg" / "packages" / name / f"{name}.abyss_a"
    path.write_bytes(b"DWELLERPKG\0" + struct.pack("<I", len(data)) + data + raw)

write_pkg("left")
write_pkg("right")
'
    HOME="$home" "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne 36 ]; then
        echo "FAIL $src: expected 36, got $actual" >&2
        exit 1
    fi
    echo "PASS $src"

    src=import_unqualified_link.as
    out="$TMPDIR/import_unqualified_link"
    HOME="$home" "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne 12 ]; then
        echo "FAIL $src: expected 12, got $actual" >&2
        exit 1
    fi
    echo "PASS $src"

    src=import_static_type_method_link.as
    out="$TMPDIR/import_static_type_method_link"
    HOME="$home" "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne 13 ]; then
        echo "FAIL $src: expected 13, got $actual" >&2
        exit 1
    fi
    echo "PASS $src"
}

run_dwellerpkg_namespace_link() {
    if [ ! -x "$DWELLERPKG" ]; then
        echo "SKIP dwellerpkg namespace link: $DWELLERPKG not built"
        return
    fi

    src=import_namespace_link.as
    home="$TMPDIR/home_dwellerpkg_namespace"
    out="$TMPDIR/dwellerpkg_namespace_link"
    left_dir="$TMPDIR/pkg_left"
    right_dir="$TMPDIR/pkg_right"

    mkdir -p "$left_dir" "$right_dir"

    cat >"$left_dir/cpkg.json" <<'EOF'
{"name":"left","version":"1.0.0"}
EOF
    cat >"$left_dir/left.as" <<'EOF'
struct Counter {
    i32 value;

    i32 read() {
        return value + 1;
    }

    static i32 base() {
        return 3;
    }
}

i32 answer() {
    return 10;
}
EOF

    cat >"$right_dir/cpkg.json" <<'EOF'
{"name":"right","version":"1.0.0"}
EOF
    cat >"$right_dir/right.as" <<'EOF'
struct Counter {
    i32 value;

    i32 read() {
        return value + 2;
    }

    static i32 base() {
        return 4;
    }
}

i32 answer() {
    return 20;
}
EOF

    (cd "$left_dir" && PATH="$ROOT/build:$PATH" HOME="$home" "$DWELLERPKG" build --static -n left -v 1.0.0 -o "$left_dir/left.abyss_a" >/dev/null)
    (cd "$right_dir" && PATH="$ROOT/build:$PATH" HOME="$home" "$DWELLERPKG" build --static -n right -v 1.0.0 -o "$right_dir/right.abyss_a" >/dev/null)
    HOME="$home" "$DWELLERPKG" install "$left_dir/left.abyss_a" left 1.0.0 >/dev/null
    HOME="$home" "$DWELLERPKG" install "$right_dir/right.abyss_a" right 1.0.0 >/dev/null

    HOME="$home" "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne 36 ]; then
        echo "FAIL dwellerpkg $src: expected 36, got $actual" >&2
        exit 1
    fi
    echo "PASS dwellerpkg $src"

    src=import_static_type_method_link.as
    out="$TMPDIR/dwellerpkg_static_type_method_link"
    HOME="$home" "$COMPILER" -link -o "$out" "$ROOT/tests/$src" >/dev/null
    set +e
    "$out" >/dev/null
    actual=$?
    set -e

    if [ "$actual" -ne 13 ]; then
        echo "FAIL dwellerpkg $src: expected 13, got $actual" >&2
        exit 1
    fi
    echo "PASS dwellerpkg $src"
}

run_dwellerpkg_interface_variadic_metadata() {
    if [ ! -x "$DWELLERPKG" ]; then
        echo "SKIP dwellerpkg interface variadic metadata: $DWELLERPKG not built"
        return
    fi

    pkg_dir="$TMPDIR/interface_variadic_pkg"
    out="$TMPDIR/logger.abyss_a"
    mkdir -p "$pkg_dir"
    PKGDIR="$pkg_dir" python3 -c '
import os
from pathlib import Path

Path(os.environ["PKGDIR"], "logger.as").write_text("""
interface Logger {
    i32 log(str fmt, ...);
}
""")
'

    (cd "$pkg_dir" && PATH="$ROOT/build:$PATH" "$DWELLERPKG" build --static -n logger -v 1.0.0 -o "$out" >/dev/null)

    meta="$TMPDIR/logger_metadata.txt"
    "$DWELLERPKG" metadata "$out" >"$meta" 2>&1
    if ! grep -Fq "i32 log(str fmt, ...)" "$meta"; then
        echo "FAIL dwellerpkg interface variadic metadata: missing variadic interface method" >&2
        exit 1
    fi
    echo "PASS dwellerpkg interface_variadic_metadata"
}

run_dwellerpkg_build_failure_status() {
    if [ ! -x "$DWELLERPKG" ]; then
        echo "SKIP dwellerpkg build failure status: $DWELLERPKG not built"
        return
    fi

    pkg_dir="$TMPDIR/bad_dwellerpkg"
    mkdir -p "$pkg_dir"
    cat >"$pkg_dir/bad.as" <<'EOF'
i32 main() {
    return ;
}
EOF

    set +e
    (cd "$pkg_dir" && PATH="$ROOT/build:$PATH" "$DWELLERPKG" build --executable >/dev/null 2>"$TMPDIR/bad_dwellerpkg.err")
    actual=$?
    set -e

    if [ "$actual" -eq 0 ]; then
        echo "FAIL dwellerpkg build failure status: expected nonzero exit" >&2
        exit 1
    fi
    echo "PASS dwellerpkg build_failure_status"
}

run_case high_priority_literals.as 42
run_case switch.as 90
run_case pointer_array.as 60
run_case else_if.as 20
run_case complete_branch_return.as 50
run_case enum_alias.as 9
run_case interface_variadic.as 37
run_case logical_short_circuit.as 0
run_case pointer_deref.as 7
run_case struct_arrow.as 7
run_case float_numeric.as 42
run_case bool_literals.as 12
run_case for_alias_decl.as 10
run_case do_while_control.as 8
run_case casts.as 13
run_case forward_calls.as 26
run_case forward_types.as 10
run_case struct_methods.as 27
run_case static_struct_methods.as 62
run_case increment_decrement.as 3
run_case struct_pointer_field.as 33
run_case recursive_structs.as 7
run_case numeric_conditions.as 21
run_case self_methods.as 47
run_case global_vars.as 28
run_case global_const_expressions.as 26
run_case static_linkage.as 26
run_case sizeof.as 23
run_case fixed_arrays.as 44
run_case array_decay.as 51
run_case conditional_expr.as 31
run_case struct_initializers.as 45
run_case nested_aggregates.as 125
run_case aggregate_arguments.as 26
run_case aggregate_assignment.as 63
run_case shift_compound_assignment.as 28
run_case interface_pointers.as 28
run_case interface_dispatch.as 42
run_ir_contains extern_globals.as "@external_counter = external global i32"
run_fail interface_mismatch_fail.as
run_fail duplicate_methods_fail.as
run_fail duplicate_interface_methods_fail.as
run_fail duplicate_params_fail.as
run_fail struct_field_method_conflict_fail.as
run_fail delete_non_pointer_fail.as
run_fail static_struct_instance_fail.as
run_fail compound_assignment_target_fail.as
run_fail bitwise_float_fail.as
run_fail shift_float_compound_fail.as
run_fail bitwise_not_float_fail.as
run_fail missing_return_fail.as
run_fail plain_missing_return_fail.as
run_fail cast_unknown_type_fail.as
run_fail cast_struct_fail.as
run_fail cast_to_void_fail.as
run_fail cast_between_structs_fail.as
run_fail new_unknown_type_fail.as
run_fail new_float_dimension_fail.as
run_fail break_outside_fail.as
run_fail continue_outside_fail.as
run_fail continue_in_switch_fail.as
run_fail sizeof_function_fail.as
run_fail sizeof_void_fail.as
run_fail index_float_fail.as
run_fail index_non_indexable_fail.as
run_fail assign_function_fail.as
run_fail compound_assign_function_fail.as
run_fail addrof_rvalue_fail.as
run_fail assign_method_fail.as
run_fail variadic_too_few_args_fail.as
run_fail unnamed_parameter_fail.as
run_fail variadic_no_fixed_param_fail.as
run_fail interface_variadic_too_few_args_fail.as
run_fail interface_variadic_no_fixed_param_fail.as
run_fail void_return_value_fail.as
run_fail missing_semicolon_fail.as
run_fail global_initializer_identifier_fail.as
run_fail global_initializer_function_fail.as
run_fail global_const_div_zero_fail.as
run_fail global_const_float_div_zero_fail.as
run_fail switch_duplicate_case_fail.as
run_fail switch_duplicate_enum_case_fail.as
run_fail unreachable_after_return_fail.as
run_fail unreachable_after_break_fail.as
run_fail unreachable_after_continue_fail.as
run_fail unreachable_after_if_return_fail.as
run_fail import_missing_fail.as
run_fail unknown_types_fail.as
run_fail generic_struct_fail.as
run_fail generic_type_args_fail.as
run_fail generic_function_fail.as
run_import_struct_metadata
run_import_rich_metadata
run_import_struct_methods_metadata
run_import_namespace_metadata
run_import_namespace_link
run_dwellerpkg_namespace_link
run_dwellerpkg_interface_variadic_metadata
run_dwellerpkg_build_failure_status
run_case comments.as 7
run_case numeric_literals.as 81
run_case char_escapes.as 150
run_case string_index.as 42
run_case string_conditions.as 42
run_case interface_return_box.as 42
run_case interface_return_delete.as 42
run_case unsigned_ops.as 37
run_case global_unsigned_const_ops.as 30
