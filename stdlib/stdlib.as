# Abyss Standard Library v0.1.0
# C function bindings
extern i32 puts(str s);
extern i32 putchar(i32 c);
extern i32 getchar();
extern i32 printf(str fmt, ...);
extern void* malloc(i64 size);
extern void* calloc(i64 count, i64 size);
extern void* realloc(void* ptr, i64 size);
extern void free(void* ptr);
extern i64 strlen(str s);
extern i32 strcmp(str a, str b);
extern i32 strncmp(str a, str b, i64 n);
extern str strchr(str s, i32 c);
extern str strstr(str haystack, str needle);
extern void* memcpy(void* dst, void* src, i64 n);
extern void* memmove(void* dst, void* src, i64 n);
extern void* memset(void* ptr, i32 val, i64 n);
extern i32 atoi(str s);
extern i64 atol(str s);
extern f64 atof(str s);
extern i32 abs(i32 x);
extern i64 llabs(i64 x);
extern i32 rand();
extern void srand(u32 seed);
extern void exit(i32 code);

# ── String utilities ────────────────────────────────────────

i64 str_len(str s) {
    return strlen(s);
}

i32 str_cmp(str a, str b) {
    return strcmp(a, b);
}

i32 str_cmp_n(str a, str b, i64 n) {
    return strncmp(a, b, n);
}

i32 str_eq(str a, str b) {
    i32 r = strcmp(a, b);
    if (r == 0) {
        return 1;
    }
    return 0;
}

i32 str_eq_n(str a, str b, i64 n) {
    i32 r = strncmp(a, b, n);
    if (r == 0) {
        return 1;
    }
    return 0;
}

i32 str_ne(str a, str b) {
    i32 r = strcmp(a, b);
    if (r != 0) {
        return 1;
    }
    return 0;
}

i32 str_is_empty(str s) {
    i32 r = strcmp(s, "");
    if (r == 0) {
        return 1;
    }
    return 0;
}

i32 str_not_empty(str s) {
    i32 r = strcmp(s, "");
    if (r != 0) {
        return 1;
    }
    return 0;
}

str str_find_char(str s, i32 c) {
    return strchr(s, c);
}

str str_find(str haystack, str needle) {
    return strstr(haystack, needle);
}

i32 str_to_i32(str s) {
    return atoi(s);
}

i64 str_to_i64(str s) {
    return atol(s);
}

f64 str_to_f64(str s) {
    return atof(s);
}

# ── Memory utilities ────────────────────────────────────────

void* mem_alloc(i64 size) {
    return malloc(size);
}

void* mem_alloc_zero(i64 count, i64 size) {
    return calloc(count, size);
}

void* mem_resize(void* ptr, i64 size) {
    return realloc(ptr, size);
}

void mem_free(void* ptr) {
    free(ptr);
}

void* mem_copy(void* dst, void* src, i64 n) {
    return memcpy(dst, src, n);
}

void* mem_move(void* dst, void* src, i64 n) {
    return memmove(dst, src, n);
}

void* mem_set(void* ptr, i32 val, i64 n) {
    return memset(ptr, val, n);
}

void* mem_zero(void* ptr, i64 n) {
    return memset(ptr, 0, n);
}

# ── Math utilities ──────────────────────────────────────────

i32 math_abs(i32 x) {
    return abs(x);
}

i64 math_abs_i64(i64 x) {
    return llabs(x);
}

i32 math_min(i32 a, i32 b) {
    if (a < b) {
        return a;
    }
    return b;
}

i32 math_max(i32 a, i32 b) {
    if (a > b) {
        return a;
    }
    return b;
}

i32 math_clamp(i32 val, i32 lo, i32 hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

i32 math_sign(i32 x) {
    if (x < 0) {
        return -1;
    }
    if (x > 0) {
        return 1;
    }
    return 0;
}

i64 math_min_i64(i64 a, i64 b) {
    if (a < b) {
        return a;
    }
    return b;
}

i64 math_max_i64(i64 a, i64 b) {
    if (a > b) {
        return a;
    }
    return b;
}

i64 math_clamp_i64(i64 val, i64 lo, i64 hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

i32 bool_not(i32 value) {
    if (!value) {
        return 1;
    }
    return 0;
}

i32 bool_and(i32 a, i32 b) {
    if (a) {
        if (b) {
            return 1;
        }
    }
    return 0;
}

i32 bool_or(i32 a, i32 b) {
    if (a) {
        return 1;
    }
    if (b) {
        return 1;
    }
    return 0;
}

# ── Print utilities ─────────────────────────────────────────

void print(str s) {
    printf("%s", s);
}

void println(str s) {
    puts(s);
}

void print_char(i32 c) {
    putchar(c);
}

void print_i32(i32 n) {
    printf("%d\n", n);
}

void print_i32_raw(i32 n) {
    printf("%d", n);
}

void print_i64(i64 n) {
    printf("%ld\n", n);
}

void print_i64_raw(i64 n) {
    printf("%ld", n);
}

void print_str(str s) {
    printf("%s\n", s);
}

void print_ptr(void* p) {
    printf("%p\n", p);
}

# ── Runtime utilities ───────────────────────────────────────

i32 input_char() {
    return getchar();
}

i32 random_i32() {
    return rand();
}

void random_seed(u32 seed) {
    srand(seed);
}

void process_exit(i32 code) {
    exit(code);
}

# ── Assert ──────────────────────────────────────────────────

void assert_fail(str msg) {
    puts("ASSERTION FAILED:");
    puts(msg);
    exit(1);
}

void assert_true(i32 cond, str msg) {
    if (cond) {
        return;
    }
    assert_fail(msg);
}

void assert_false(i32 cond, str msg) {
    if (!cond) {
        return;
    }
    assert_fail(msg);
}
