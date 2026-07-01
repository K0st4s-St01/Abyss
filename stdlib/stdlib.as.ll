; ModuleID = 'stdlib.as'
source_filename = "stdlib.as"

@.str = private constant [1 x i8] zeroinitializer
@.str.1 = private constant [1 x i8] zeroinitializer
@.str.2 = private constant [3 x i8] c"%s\00"
@.str.3 = private constant [4 x i8] c"%d\0A\00"
@.str.4 = private constant [3 x i8] c"%d\00"
@.str.5 = private constant [5 x i8] c"%ld\0A\00"
@.str.6 = private constant [4 x i8] c"%ld\00"
@.str.7 = private constant [4 x i8] c"%s\0A\00"
@.str.8 = private constant [4 x i8] c"%p\0A\00"
@.str.9 = private constant [18 x i8] c"ASSERTION FAILED:\00"

@str_len = weak alias i64 (ptr), ptr @stdlib.str_len
@str_cmp = weak alias i32 (ptr, ptr), ptr @stdlib.str_cmp
@str_cmp_n = weak alias i32 (ptr, ptr, i64), ptr @stdlib.str_cmp_n
@str_eq = weak alias i32 (ptr, ptr), ptr @stdlib.str_eq
@str_eq_n = weak alias i32 (ptr, ptr, i64), ptr @stdlib.str_eq_n
@str_ne = weak alias i32 (ptr, ptr), ptr @stdlib.str_ne
@str_is_empty = weak alias i32 (ptr), ptr @stdlib.str_is_empty
@str_not_empty = weak alias i32 (ptr), ptr @stdlib.str_not_empty
@str_find_char = weak alias ptr (ptr, i32), ptr @stdlib.str_find_char
@str_find = weak alias ptr (ptr, ptr), ptr @stdlib.str_find
@str_contains = weak alias i32 (ptr, ptr), ptr @stdlib.str_contains
@str_starts_with = weak alias i32 (ptr, ptr), ptr @stdlib.str_starts_with
@str_ends_with = weak alias i32 (ptr, ptr), ptr @stdlib.str_ends_with
@str_index_of = weak alias i32 (ptr, ptr), ptr @stdlib.str_index_of
@str_char_at = weak alias i32 (ptr, i64), ptr @stdlib.str_char_at
@str_to_i32 = weak alias i32 (ptr), ptr @stdlib.str_to_i32
@str_to_i64 = weak alias i64 (ptr), ptr @stdlib.str_to_i64
@str_to_f64 = weak alias double (ptr), ptr @stdlib.str_to_f64
@mem_alloc = weak alias ptr (i64), ptr @stdlib.mem_alloc
@mem_alloc_zero = weak alias ptr (i64, i64), ptr @stdlib.mem_alloc_zero
@mem_resize = weak alias ptr (ptr, i64), ptr @stdlib.mem_resize
@mem_free = weak alias void (ptr), ptr @stdlib.mem_free
@mem_copy = weak alias ptr (ptr, ptr, i64), ptr @stdlib.mem_copy
@mem_move = weak alias ptr (ptr, ptr, i64), ptr @stdlib.mem_move
@mem_set = weak alias ptr (ptr, i32, i64), ptr @stdlib.mem_set
@mem_zero = weak alias ptr (ptr, i64), ptr @stdlib.mem_zero
@mem_compare = weak alias i32 (ptr, ptr, i64), ptr @stdlib.mem_compare
@mem_eq = weak alias i32 (ptr, ptr, i64), ptr @stdlib.mem_eq
@math_abs = weak alias i32 (i32), ptr @stdlib.math_abs
@math_abs_i64 = weak alias i64 (i64), ptr @stdlib.math_abs_i64
@math_min = weak alias i32 (i32, i32), ptr @stdlib.math_min
@math_max = weak alias i32 (i32, i32), ptr @stdlib.math_max
@math_clamp = weak alias i32 (i32, i32, i32), ptr @stdlib.math_clamp
@math_sign = weak alias i32 (i32), ptr @stdlib.math_sign
@math_min_i64 = weak alias i64 (i64, i64), ptr @stdlib.math_min_i64
@math_max_i64 = weak alias i64 (i64, i64), ptr @stdlib.math_max_i64
@math_clamp_i64 = weak alias i64 (i64, i64, i64), ptr @stdlib.math_clamp_i64
@math_abs_f64 = weak alias double (double), ptr @stdlib.math_abs_f64
@math_sqrt = weak alias double (double), ptr @stdlib.math_sqrt
@math_pow = weak alias double (double, double), ptr @stdlib.math_pow
@math_floor = weak alias double (double), ptr @stdlib.math_floor
@math_ceil = weak alias double (double), ptr @stdlib.math_ceil
@math_min_f64 = weak alias double (double, double), ptr @stdlib.math_min_f64
@math_max_f64 = weak alias double (double, double), ptr @stdlib.math_max_f64
@math_clamp_f64 = weak alias double (double, double, double), ptr @stdlib.math_clamp_f64
@math_in_range = weak alias i32 (i32, i32, i32), ptr @stdlib.math_in_range
@math_in_range_i64 = weak alias i32 (i64, i64, i64), ptr @stdlib.math_in_range_i64
@char_is_alpha = weak alias i32 (i32), ptr @stdlib.char_is_alpha
@char_is_digit = weak alias i32 (i32), ptr @stdlib.char_is_digit
@char_is_alnum = weak alias i32 (i32), ptr @stdlib.char_is_alnum
@char_is_space = weak alias i32 (i32), ptr @stdlib.char_is_space
@char_to_upper = weak alias i32 (i32), ptr @stdlib.char_to_upper
@char_to_lower = weak alias i32 (i32), ptr @stdlib.char_to_lower
@bool_not = weak alias i32 (i32), ptr @stdlib.bool_not
@bool_and = weak alias i32 (i32, i32), ptr @stdlib.bool_and
@bool_or = weak alias i32 (i32, i32), ptr @stdlib.bool_or
@print = weak alias void (ptr), ptr @stdlib.print
@println = weak alias void (ptr), ptr @stdlib.println
@print_char = weak alias void (i32), ptr @stdlib.print_char
@print_i32 = weak alias void (i32), ptr @stdlib.print_i32
@print_i32_raw = weak alias void (i32), ptr @stdlib.print_i32_raw
@print_i64 = weak alias void (i64), ptr @stdlib.print_i64
@print_i64_raw = weak alias void (i64), ptr @stdlib.print_i64_raw
@print_str = weak alias void (ptr), ptr @stdlib.print_str
@print_ptr = weak alias void (ptr), ptr @stdlib.print_ptr
@input_char = weak alias i32 (), ptr @stdlib.input_char
@random_i32 = weak alias i32 (), ptr @stdlib.random_i32
@random_seed = weak alias void (i32), ptr @stdlib.random_seed
@process_exit = weak alias void (i32), ptr @stdlib.process_exit
@assert_fail = weak alias void (ptr), ptr @stdlib.assert_fail
@assert_true = weak alias void (i32, ptr), ptr @stdlib.assert_true
@assert_false = weak alias void (i32, ptr), ptr @stdlib.assert_false

declare i32 @puts(ptr)

declare i32 @putchar(i32)

declare i32 @getchar()

declare i32 @printf(ptr, ...)

declare ptr @malloc(i64)

declare ptr @calloc(i64, i64)

declare ptr @realloc(ptr, i64)

declare void @free(ptr)

declare i64 @strlen(ptr)

declare i32 @strcmp(ptr, ptr)

declare i32 @strncmp(ptr, ptr, i64)

declare ptr @strchr(ptr, i32)

declare ptr @strstr(ptr, ptr)

declare ptr @memcpy(ptr, ptr, i64)

declare ptr @memmove(ptr, ptr, i64)

declare ptr @memset(ptr, i32, i64)

declare i32 @memcmp(ptr, ptr, i64)

declare i32 @atoi(ptr)

declare i64 @atol(ptr)

declare double @atof(ptr)

declare i32 @abs(i32)

declare i64 @llabs(i64)

declare double @fabs(double)

declare double @sqrt(double)

declare double @pow(double, double)

declare double @floor(double)

declare double @ceil(double)

declare i32 @isalpha(i32)

declare i32 @isdigit(i32)

declare i32 @isalnum(i32)

declare i32 @isspace(i32)

declare i32 @toupper(i32)

declare i32 @tolower(i32)

declare i32 @rand()

declare void @srand(i32)

declare void @exit(i32)

define i64 @stdlib.str_len(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i64 @strlen(ptr %arg)
  ret i64 %call
}

define i32 @stdlib.str_cmp(ptr %a, ptr %b) {
entry:
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %arg = load ptr, ptr %a1, align 8
  %arg3 = load ptr, ptr %b2, align 8
  %call = call i32 @strcmp(ptr %arg, ptr %arg3)
  ret i32 %call
}

define i32 @stdlib.str_cmp_n(ptr %a, ptr %b, i64 %n) {
entry:
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %a1, align 8
  %arg4 = load ptr, ptr %b2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call i32 @strncmp(ptr %arg, ptr %arg4, i64 %arg5)
  ret i32 %call
}

define i32 @stdlib.str_eq(ptr %a, ptr %b) {
entry:
  %r = alloca i32, align 4
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %arg = load ptr, ptr %a1, align 8
  %arg3 = load ptr, ptr %b2, align 8
  %call = call i32 @strcmp(ptr %arg, ptr %arg3)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_eq_n(ptr %a, ptr %b, i64 %n) {
entry:
  %r = alloca i32, align 4
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %a1, align 8
  %arg4 = load ptr, ptr %b2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call i32 @strncmp(ptr %arg, ptr %arg4, i64 %arg5)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_ne(ptr %a, ptr %b) {
entry:
  %r = alloca i32, align 4
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %arg = load ptr, ptr %a1, align 8
  %arg3 = load ptr, ptr %b2, align 8
  %call = call i32 @strcmp(ptr %arg, ptr %arg3)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %ne = icmp ne i32 %load, 0
  %ne_ext = zext i1 %ne to i32
  %cond = icmp ne i32 %ne_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_is_empty(ptr %s) {
entry:
  %r = alloca i32, align 4
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @strcmp(ptr %arg, ptr @.str)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_not_empty(ptr %s) {
entry:
  %r = alloca i32, align 4
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @strcmp(ptr %arg, ptr @.str.1)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %ne = icmp ne i32 %load, 0
  %ne_ext = zext i1 %ne to i32
  %cond = icmp ne i32 %ne_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define ptr @stdlib.str_find_char(ptr %s, i32 %c) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %c2 = alloca i32, align 4
  store i32 %c, ptr %c2, align 4
  %arg = load ptr, ptr %s1, align 8
  %arg3 = load i32, ptr %c2, align 4
  %call = call ptr @strchr(ptr %arg, i32 %arg3)
  ret ptr %call
}

define ptr @stdlib.str_find(ptr %haystack, ptr %needle) {
entry:
  %haystack1 = alloca ptr, align 8
  store ptr %haystack, ptr %haystack1, align 8
  %needle2 = alloca ptr, align 8
  store ptr %needle, ptr %needle2, align 8
  %arg = load ptr, ptr %haystack1, align 8
  %arg3 = load ptr, ptr %needle2, align 8
  %call = call ptr @strstr(ptr %arg, ptr %arg3)
  ret ptr %call
}

define i32 @stdlib.str_contains(ptr %haystack, ptr %needle) {
entry:
  %found = alloca ptr, align 8
  %haystack1 = alloca ptr, align 8
  store ptr %haystack, ptr %haystack1, align 8
  %needle2 = alloca ptr, align 8
  store ptr %needle, ptr %needle2, align 8
  %arg = load ptr, ptr %haystack1, align 8
  %arg3 = load ptr, ptr %needle2, align 8
  %call = call ptr @strstr(ptr %arg, ptr %arg3)
  store ptr %call, ptr %found, align 8
  %load = load ptr, ptr %found, align 8
  %cond = icmp ne ptr %load, null
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_starts_with(ptr %s, ptr %prefix) {
entry:
  %r = alloca i32, align 4
  %n = alloca i64, align 8
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %prefix2 = alloca ptr, align 8
  store ptr %prefix, ptr %prefix2, align 8
  %arg = load ptr, ptr %prefix2, align 8
  %call = call i64 @strlen(ptr %arg)
  store i64 %call, ptr %n, align 4
  %arg3 = load ptr, ptr %s1, align 8
  %arg4 = load ptr, ptr %prefix2, align 8
  %arg5 = load i64, ptr %n, align 4
  %call6 = call i32 @strncmp(ptr %arg3, ptr %arg4, i64 %arg5)
  store i32 %call6, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.str_ends_with(ptr %s, ptr %suffix) {
entry:
  %i = alloca i64, align 8
  %offset = alloca i64, align 8
  %suffix_len = alloca i64, align 8
  %len = alloca i64, align 8
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %suffix2 = alloca ptr, align 8
  store ptr %suffix, ptr %suffix2, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i64 @strlen(ptr %arg)
  store i64 %call, ptr %len, align 4
  %arg3 = load ptr, ptr %suffix2, align 8
  %call4 = call i64 @strlen(ptr %arg3)
  store i64 %call4, ptr %suffix_len, align 4
  %load = load i64, ptr %suffix_len, align 4
  %load5 = load i64, ptr %len, align 4
  %sgt = icmp sgt i64 %load, %load5
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 0

if.end:                                           ; preds = %entry
  %load6 = load i64, ptr %len, align 4
  %load7 = load i64, ptr %suffix_len, align 4
  %sub = sub i64 %load6, %load7
  store i64 %sub, ptr %offset, align 4
  store i64 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %if.end21, %if.end
  %load8 = load i64, ptr %i, align 4
  %load9 = load i64, ptr %suffix_len, align 4
  %slt = icmp slt i64 %load8, %load9
  %slt_ext = zext i1 %slt to i32
  %cond10 = icmp ne i32 %slt_ext, 0
  br i1 %cond10, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %load11 = load i64, ptr %offset, align 4
  %load12 = load i64, ptr %i, align 4
  %add = add i64 %load11, %load12
  %deref = load ptr, ptr %s1, align 8
  %idx = getelementptr i8, ptr %deref, i64 %add
  %load13 = load i8, ptr %idx, align 1
  %sext = sext i8 %load13 to i32
  %load14 = load i64, ptr %i, align 4
  %deref15 = load ptr, ptr %suffix2, align 8
  %idx16 = getelementptr i8, ptr %deref15, i64 %load14
  %load17 = load i8, ptr %idx16, align 1
  %sext18 = sext i8 %load17 to i32
  %ne = icmp ne i32 %sext, %sext18
  %ne_ext = zext i1 %ne to i32
  %cond19 = icmp ne i32 %ne_ext, 0
  br i1 %cond19, label %if.then20, label %if.end21

while.end:                                        ; preds = %while.cond
  ret i32 1

if.then20:                                        ; preds = %while.body
  ret i32 0

if.end21:                                         ; preds = %while.body
  %cur = load i64, ptr %i, align 4
  %add22 = add i64 %cur, 1
  store i64 %add22, ptr %i, align 4
  br label %while.cond
}

define i32 @stdlib.str_index_of(ptr %haystack, ptr %needle) {
entry:
  %j = alloca i64, align 8
  %i = alloca i32, align 4
  %needle_len = alloca i64, align 8
  %haystack1 = alloca ptr, align 8
  store ptr %haystack, ptr %haystack1, align 8
  %needle2 = alloca ptr, align 8
  store ptr %needle, ptr %needle2, align 8
  %arg = load ptr, ptr %needle2, align 8
  %call = call i64 @strlen(ptr %arg)
  store i64 %call, ptr %needle_len, align 4
  %load = load i64, ptr %needle_len, align 4
  %eq = icmp eq i64 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 0

if.end:                                           ; preds = %entry
  store i32 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %if.end49, %if.end
  %load3 = load i32, ptr %i, align 4
  %deref = load ptr, ptr %haystack1, align 8
  %idx = getelementptr i8, ptr %deref, i32 %load3
  %load4 = load i8, ptr %idx, align 1
  %sext = sext i8 %load4 to i32
  %ne = icmp ne i32 %sext, 0
  %ne_ext = zext i1 %ne to i32
  %cond5 = icmp ne i32 %ne_ext, 0
  br i1 %cond5, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  store i64 0, ptr %j, align 4
  br label %while.cond6

while.end:                                        ; preds = %while.cond
  ret i32 -1

while.cond6:                                      ; preds = %if.end41, %while.body
  %load9 = load i64, ptr %j, align 4
  %load10 = load i64, ptr %needle_len, align 4
  %slt = icmp slt i64 %load9, %load10
  %slt_ext = zext i1 %slt to i32
  %cond11 = icmp ne i32 %slt_ext, 0
  br i1 %cond11, label %while.body7, label %while.end8

while.body7:                                      ; preds = %while.cond6
  %load12 = load i32, ptr %i, align 4
  %load13 = load i64, ptr %j, align 4
  %sext14 = sext i32 %load12 to i64
  %add = add i64 %sext14, %load13
  %deref15 = load ptr, ptr %haystack1, align 8
  %idx16 = getelementptr i8, ptr %deref15, i64 %add
  %load17 = load i8, ptr %idx16, align 1
  %sext18 = sext i8 %load17 to i32
  %eq19 = icmp eq i32 %sext18, 0
  %eq_ext20 = zext i1 %eq19 to i32
  %cond21 = icmp ne i32 %eq_ext20, 0
  br i1 %cond21, label %if.then22, label %if.end23

while.end8:                                       ; preds = %if.then40, %while.cond6
  %load43 = load i64, ptr %j, align 4
  %load44 = load i64, ptr %needle_len, align 4
  %eq45 = icmp eq i64 %load43, %load44
  %eq_ext46 = zext i1 %eq45 to i32
  %cond47 = icmp ne i32 %eq_ext46, 0
  br i1 %cond47, label %if.then48, label %if.end49

if.then22:                                        ; preds = %while.body7
  ret i32 -1

if.end23:                                         ; preds = %while.body7
  %load24 = load i32, ptr %i, align 4
  %load25 = load i64, ptr %j, align 4
  %sext26 = sext i32 %load24 to i64
  %add27 = add i64 %sext26, %load25
  %deref28 = load ptr, ptr %haystack1, align 8
  %idx29 = getelementptr i8, ptr %deref28, i64 %add27
  %load30 = load i8, ptr %idx29, align 1
  %sext31 = sext i8 %load30 to i32
  %load32 = load i64, ptr %j, align 4
  %deref33 = load ptr, ptr %needle2, align 8
  %idx34 = getelementptr i8, ptr %deref33, i64 %load32
  %load35 = load i8, ptr %idx34, align 1
  %sext36 = sext i8 %load35 to i32
  %ne37 = icmp ne i32 %sext31, %sext36
  %ne_ext38 = zext i1 %ne37 to i32
  %cond39 = icmp ne i32 %ne_ext38, 0
  br i1 %cond39, label %if.then40, label %if.end41

if.then40:                                        ; preds = %if.end23
  br label %while.end8

if.end41:                                         ; preds = %if.end23
  %cur = load i64, ptr %j, align 4
  %add42 = add i64 %cur, 1
  store i64 %add42, ptr %j, align 4
  br label %while.cond6

if.then48:                                        ; preds = %while.end8
  %load50 = load i32, ptr %i, align 4
  ret i32 %load50

if.end49:                                         ; preds = %while.end8
  %cur51 = load i32, ptr %i, align 4
  %add52 = add i32 %cur51, 1
  store i32 %add52, ptr %i, align 4
  br label %while.cond
}

define i32 @stdlib.str_char_at(ptr %s, i64 %index) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %index2 = alloca i64, align 8
  store i64 %index, ptr %index2, align 4
  %load = load i64, ptr %index2, align 4
  %deref = load ptr, ptr %s1, align 8
  %idx = getelementptr i8, ptr %deref, i64 %load
  %load3 = load i8, ptr %idx, align 1
  %sext = sext i8 %load3 to i32
  ret i32 %sext
}

define i32 @stdlib.str_to_i32(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @atoi(ptr %arg)
  ret i32 %call
}

define i64 @stdlib.str_to_i64(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i64 @atol(ptr %arg)
  ret i64 %call
}

define double @stdlib.str_to_f64(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call double @atof(ptr %arg)
  ret double %call
}

define ptr @stdlib.mem_alloc(i64 %size) {
entry:
  %size1 = alloca i64, align 8
  store i64 %size, ptr %size1, align 4
  %arg = load i64, ptr %size1, align 4
  %call = call ptr @malloc(i64 %arg)
  ret ptr %call
}

define ptr @stdlib.mem_alloc_zero(i64 %count, i64 %size) {
entry:
  %count1 = alloca i64, align 8
  store i64 %count, ptr %count1, align 4
  %size2 = alloca i64, align 8
  store i64 %size, ptr %size2, align 4
  %arg = load i64, ptr %count1, align 4
  %arg3 = load i64, ptr %size2, align 4
  %call = call ptr @calloc(i64 %arg, i64 %arg3)
  ret ptr %call
}

define ptr @stdlib.mem_resize(ptr %ptr, i64 %size) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %size2 = alloca i64, align 8
  store i64 %size, ptr %size2, align 4
  %arg = load ptr, ptr %ptr1, align 8
  %arg3 = load i64, ptr %size2, align 4
  %call = call ptr @realloc(ptr %arg, i64 %arg3)
  ret ptr %call
}

define void @stdlib.mem_free(ptr %ptr) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %arg = load ptr, ptr %ptr1, align 8
  call void @free(ptr %arg)
  ret void
}

define ptr @stdlib.mem_copy(ptr %dst, ptr %src, i64 %n) {
entry:
  %dst1 = alloca ptr, align 8
  store ptr %dst, ptr %dst1, align 8
  %src2 = alloca ptr, align 8
  store ptr %src, ptr %src2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %dst1, align 8
  %arg4 = load ptr, ptr %src2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memcpy(ptr %arg, ptr %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @stdlib.mem_move(ptr %dst, ptr %src, i64 %n) {
entry:
  %dst1 = alloca ptr, align 8
  store ptr %dst, ptr %dst1, align 8
  %src2 = alloca ptr, align 8
  store ptr %src, ptr %src2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %dst1, align 8
  %arg4 = load ptr, ptr %src2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memmove(ptr %arg, ptr %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @stdlib.mem_set(ptr %ptr, i32 %val, i64 %n) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %val2 = alloca i32, align 4
  store i32 %val, ptr %val2, align 4
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %ptr1, align 8
  %arg4 = load i32, ptr %val2, align 4
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memset(ptr %arg, i32 %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @stdlib.mem_zero(ptr %ptr, i64 %n) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %n2 = alloca i64, align 8
  store i64 %n, ptr %n2, align 4
  %arg = load ptr, ptr %ptr1, align 8
  %arg3 = load i64, ptr %n2, align 4
  %call = call ptr @memset(ptr %arg, i32 0, i64 %arg3)
  ret ptr %call
}

define i32 @stdlib.mem_compare(ptr %a, ptr %b, i64 %n) {
entry:
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %a1, align 8
  %arg4 = load ptr, ptr %b2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call i32 @memcmp(ptr %arg, ptr %arg4, i64 %arg5)
  ret i32 %call
}

define i32 @stdlib.mem_eq(ptr %a, ptr %b, i64 %n) {
entry:
  %r = alloca i32, align 4
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  %b2 = alloca ptr, align 8
  store ptr %b, ptr %b2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load ptr, ptr %a1, align 8
  %arg4 = load ptr, ptr %b2, align 8
  %arg5 = load i64, ptr %n3, align 4
  %call = call i32 @memcmp(ptr %arg, ptr %arg4, i64 %arg5)
  store i32 %call, ptr %r, align 4
  %load = load i32, ptr %r, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.math_abs(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %arg = load i32, ptr %x1, align 4
  %call = call i32 @abs(i32 %arg)
  ret i32 %call
}

define i64 @stdlib.math_abs_i64(i64 %x) {
entry:
  %x1 = alloca i64, align 8
  store i64 %x, ptr %x1, align 4
  %arg = load i64, ptr %x1, align 4
  %call = call i64 @llabs(i64 %arg)
  ret i64 %call
}

define i32 @stdlib.math_min(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %load = load i32, ptr %a1, align 4
  %load3 = load i32, ptr %b2, align 4
  %slt = icmp slt i32 %load, %load3
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load i32, ptr %a1, align 4
  ret i32 %load4

if.end:                                           ; preds = %entry
  %load5 = load i32, ptr %b2, align 4
  ret i32 %load5
}

define i32 @stdlib.math_max(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %load = load i32, ptr %a1, align 4
  %load3 = load i32, ptr %b2, align 4
  %sgt = icmp sgt i32 %load, %load3
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load i32, ptr %a1, align 4
  ret i32 %load4

if.end:                                           ; preds = %entry
  %load5 = load i32, ptr %b2, align 4
  ret i32 %load5
}

define i32 @stdlib.math_clamp(i32 %val, i32 %lo, i32 %hi) {
entry:
  %val1 = alloca i32, align 4
  store i32 %val, ptr %val1, align 4
  %lo2 = alloca i32, align 4
  store i32 %lo, ptr %lo2, align 4
  %hi3 = alloca i32, align 4
  store i32 %hi, ptr %hi3, align 4
  %load = load i32, ptr %val1, align 4
  %load4 = load i32, ptr %lo2, align 4
  %slt = icmp slt i32 %load, %load4
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load5 = load i32, ptr %lo2, align 4
  ret i32 %load5

if.end:                                           ; preds = %entry
  %load6 = load i32, ptr %val1, align 4
  %load7 = load i32, ptr %hi3, align 4
  %sgt = icmp sgt i32 %load6, %load7
  %sgt_ext = zext i1 %sgt to i32
  %cond8 = icmp ne i32 %sgt_ext, 0
  br i1 %cond8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %if.end
  %load11 = load i32, ptr %hi3, align 4
  ret i32 %load11

if.end10:                                         ; preds = %if.end
  %load12 = load i32, ptr %val1, align 4
  ret i32 %load12
}

define i32 @stdlib.math_sign(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %load = load i32, ptr %x1, align 4
  %slt = icmp slt i32 %load, 0
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 -1

if.end:                                           ; preds = %entry
  %load2 = load i32, ptr %x1, align 4
  %sgt = icmp sgt i32 %load2, 0
  %sgt_ext = zext i1 %sgt to i32
  %cond3 = icmp ne i32 %sgt_ext, 0
  br i1 %cond3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %if.end
  ret i32 1

if.end5:                                          ; preds = %if.end
  ret i32 0
}

define i64 @stdlib.math_min_i64(i64 %a, i64 %b) {
entry:
  %a1 = alloca i64, align 8
  store i64 %a, ptr %a1, align 4
  %b2 = alloca i64, align 8
  store i64 %b, ptr %b2, align 4
  %load = load i64, ptr %a1, align 4
  %load3 = load i64, ptr %b2, align 4
  %slt = icmp slt i64 %load, %load3
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load i64, ptr %a1, align 4
  ret i64 %load4

if.end:                                           ; preds = %entry
  %load5 = load i64, ptr %b2, align 4
  ret i64 %load5
}

define i64 @stdlib.math_max_i64(i64 %a, i64 %b) {
entry:
  %a1 = alloca i64, align 8
  store i64 %a, ptr %a1, align 4
  %b2 = alloca i64, align 8
  store i64 %b, ptr %b2, align 4
  %load = load i64, ptr %a1, align 4
  %load3 = load i64, ptr %b2, align 4
  %sgt = icmp sgt i64 %load, %load3
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load i64, ptr %a1, align 4
  ret i64 %load4

if.end:                                           ; preds = %entry
  %load5 = load i64, ptr %b2, align 4
  ret i64 %load5
}

define i64 @stdlib.math_clamp_i64(i64 %val, i64 %lo, i64 %hi) {
entry:
  %val1 = alloca i64, align 8
  store i64 %val, ptr %val1, align 4
  %lo2 = alloca i64, align 8
  store i64 %lo, ptr %lo2, align 4
  %hi3 = alloca i64, align 8
  store i64 %hi, ptr %hi3, align 4
  %load = load i64, ptr %val1, align 4
  %load4 = load i64, ptr %lo2, align 4
  %slt = icmp slt i64 %load, %load4
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load5 = load i64, ptr %lo2, align 4
  ret i64 %load5

if.end:                                           ; preds = %entry
  %load6 = load i64, ptr %val1, align 4
  %load7 = load i64, ptr %hi3, align 4
  %sgt = icmp sgt i64 %load6, %load7
  %sgt_ext = zext i1 %sgt to i32
  %cond8 = icmp ne i32 %sgt_ext, 0
  br i1 %cond8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %if.end
  %load11 = load i64, ptr %hi3, align 4
  ret i64 %load11

if.end10:                                         ; preds = %if.end
  %load12 = load i64, ptr %val1, align 4
  ret i64 %load12
}

define double @stdlib.math_abs_f64(double %x) {
entry:
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  %arg = load double, ptr %x1, align 8
  %call = call double @fabs(double %arg)
  ret double %call
}

define double @stdlib.math_sqrt(double %x) {
entry:
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  %arg = load double, ptr %x1, align 8
  %call = call double @sqrt(double %arg)
  ret double %call
}

define double @stdlib.math_pow(double %base, double %exp) {
entry:
  %base1 = alloca double, align 8
  store double %base, ptr %base1, align 8
  %exp2 = alloca double, align 8
  store double %exp, ptr %exp2, align 8
  %arg = load double, ptr %base1, align 8
  %arg3 = load double, ptr %exp2, align 8
  %call = call double @pow(double %arg, double %arg3)
  ret double %call
}

define double @stdlib.math_floor(double %x) {
entry:
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  %arg = load double, ptr %x1, align 8
  %call = call double @floor(double %arg)
  ret double %call
}

define double @stdlib.math_ceil(double %x) {
entry:
  %x1 = alloca double, align 8
  store double %x, ptr %x1, align 8
  %arg = load double, ptr %x1, align 8
  %call = call double @ceil(double %arg)
  ret double %call
}

define double @stdlib.math_min_f64(double %a, double %b) {
entry:
  %a1 = alloca double, align 8
  store double %a, ptr %a1, align 8
  %b2 = alloca double, align 8
  store double %b, ptr %b2, align 8
  %load = load double, ptr %a1, align 8
  %load3 = load double, ptr %b2, align 8
  %flt = fcmp olt double %load, %load3
  %slt_ext = zext i1 %flt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load double, ptr %a1, align 8
  ret double %load4

if.end:                                           ; preds = %entry
  %load5 = load double, ptr %b2, align 8
  ret double %load5
}

define double @stdlib.math_max_f64(double %a, double %b) {
entry:
  %a1 = alloca double, align 8
  store double %a, ptr %a1, align 8
  %b2 = alloca double, align 8
  store double %b, ptr %b2, align 8
  %load = load double, ptr %a1, align 8
  %load3 = load double, ptr %b2, align 8
  %fgt = fcmp ogt double %load, %load3
  %sgt_ext = zext i1 %fgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load4 = load double, ptr %a1, align 8
  ret double %load4

if.end:                                           ; preds = %entry
  %load5 = load double, ptr %b2, align 8
  ret double %load5
}

define double @stdlib.math_clamp_f64(double %val, double %lo, double %hi) {
entry:
  %val1 = alloca double, align 8
  store double %val, ptr %val1, align 8
  %lo2 = alloca double, align 8
  store double %lo, ptr %lo2, align 8
  %hi3 = alloca double, align 8
  store double %hi, ptr %hi3, align 8
  %load = load double, ptr %val1, align 8
  %load4 = load double, ptr %lo2, align 8
  %flt = fcmp olt double %load, %load4
  %slt_ext = zext i1 %flt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load5 = load double, ptr %lo2, align 8
  ret double %load5

if.end:                                           ; preds = %entry
  %load6 = load double, ptr %val1, align 8
  %load7 = load double, ptr %hi3, align 8
  %fgt = fcmp ogt double %load6, %load7
  %sgt_ext = zext i1 %fgt to i32
  %cond8 = icmp ne i32 %sgt_ext, 0
  br i1 %cond8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %if.end
  %load11 = load double, ptr %hi3, align 8
  ret double %load11

if.end10:                                         ; preds = %if.end
  %load12 = load double, ptr %val1, align 8
  ret double %load12
}

define i32 @stdlib.math_in_range(i32 %val, i32 %lo, i32 %hi) {
entry:
  %val1 = alloca i32, align 4
  store i32 %val, ptr %val1, align 4
  %lo2 = alloca i32, align 4
  store i32 %lo, ptr %lo2, align 4
  %hi3 = alloca i32, align 4
  store i32 %hi, ptr %hi3, align 4
  %load = load i32, ptr %val1, align 4
  %load4 = load i32, ptr %lo2, align 4
  %slt = icmp slt i32 %load, %load4
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 0

if.end:                                           ; preds = %entry
  %load5 = load i32, ptr %val1, align 4
  %load6 = load i32, ptr %hi3, align 4
  %sgt = icmp sgt i32 %load5, %load6
  %sgt_ext = zext i1 %sgt to i32
  %cond7 = icmp ne i32 %sgt_ext, 0
  br i1 %cond7, label %if.then8, label %if.end9

if.then8:                                         ; preds = %if.end
  ret i32 0

if.end9:                                          ; preds = %if.end
  ret i32 1
}

define i32 @stdlib.math_in_range_i64(i64 %val, i64 %lo, i64 %hi) {
entry:
  %val1 = alloca i64, align 8
  store i64 %val, ptr %val1, align 4
  %lo2 = alloca i64, align 8
  store i64 %lo, ptr %lo2, align 4
  %hi3 = alloca i64, align 8
  store i64 %hi, ptr %hi3, align 4
  %load = load i64, ptr %val1, align 4
  %load4 = load i64, ptr %lo2, align 4
  %slt = icmp slt i64 %load, %load4
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 0

if.end:                                           ; preds = %entry
  %load5 = load i64, ptr %val1, align 4
  %load6 = load i64, ptr %hi3, align 4
  %sgt = icmp sgt i64 %load5, %load6
  %sgt_ext = zext i1 %sgt to i32
  %cond7 = icmp ne i32 %sgt_ext, 0
  br i1 %cond7, label %if.then8, label %if.end9

if.then8:                                         ; preds = %if.end
  ret i32 0

if.end9:                                          ; preds = %if.end
  ret i32 1
}

define i32 @stdlib.char_is_alpha(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @isalpha(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.char_is_digit(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @isdigit(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.char_is_alnum(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @isalnum(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.char_is_space(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @isspace(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.char_to_upper(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @toupper(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.char_to_lower(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @tolower(i32 %arg)
  ret i32 %call
}

define i32 @stdlib.bool_not(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %cond = icmp ne i32 %load, 0
  %not = icmp eq i1 %cond, false
  %not_ext = zext i1 %not to i32
  %cond2 = icmp ne i32 %not_ext, 0
  br i1 %cond2, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @stdlib.bool_and(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %load = load i32, ptr %a1, align 4
  %cond = icmp ne i32 %load, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load3 = load i32, ptr %b2, align 4
  %cond4 = icmp ne i32 %load3, 0
  br i1 %cond4, label %if.then5, label %if.end6

if.end:                                           ; preds = %if.end6, %entry
  ret i32 0

if.then5:                                         ; preds = %if.then
  ret i32 1

if.end6:                                          ; preds = %if.then
  br label %if.end
}

define i32 @stdlib.bool_or(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, ptr %b2, align 4
  %load = load i32, ptr %a1, align 4
  %cond = icmp ne i32 %load, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  %load3 = load i32, ptr %b2, align 4
  %cond4 = icmp ne i32 %load3, 0
  br i1 %cond4, label %if.then5, label %if.end6

if.then5:                                         ; preds = %if.end
  ret i32 1

if.end6:                                          ; preds = %if.end
  ret i32 0
}

define void @stdlib.print(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 (ptr, ...) @printf(ptr @.str.2, ptr %arg)
  ret void
}

define void @stdlib.println(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @puts(ptr %arg)
  ret void
}

define void @stdlib.print_char(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @putchar(i32 %arg)
  ret void
}

define void @stdlib.print_i32(i32 %n) {
entry:
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %arg = load i32, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.3, i32 %arg)
  ret void
}

define void @stdlib.print_i32_raw(i32 %n) {
entry:
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %arg = load i32, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.4, i32 %arg)
  ret void
}

define void @stdlib.print_i64(i64 %n) {
entry:
  %n1 = alloca i64, align 8
  store i64 %n, ptr %n1, align 4
  %arg = load i64, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.5, i64 %arg)
  ret void
}

define void @stdlib.print_i64_raw(i64 %n) {
entry:
  %n1 = alloca i64, align 8
  store i64 %n, ptr %n1, align 4
  %arg = load i64, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.6, i64 %arg)
  ret void
}

define void @stdlib.print_str(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %arg)
  ret void
}

define void @stdlib.print_ptr(ptr %p) {
entry:
  %p1 = alloca ptr, align 8
  store ptr %p, ptr %p1, align 8
  %arg = load ptr, ptr %p1, align 8
  %call = call i32 (ptr, ...) @printf(ptr @.str.8, ptr %arg)
  ret void
}

define i32 @stdlib.input_char() {
entry:
  %call = call i32 @getchar()
  ret i32 %call
}

define i32 @stdlib.random_i32() {
entry:
  %call = call i32 @rand()
  ret i32 %call
}

define void @stdlib.random_seed(i32 %seed) {
entry:
  %seed1 = alloca i32, align 4
  store i32 %seed, ptr %seed1, align 4
  %arg = load i32, ptr %seed1, align 4
  call void @srand(i32 %arg)
  ret void
}

define void @stdlib.process_exit(i32 %code) {
entry:
  %code1 = alloca i32, align 4
  store i32 %code, ptr %code1, align 4
  %arg = load i32, ptr %code1, align 4
  call void @exit(i32 %arg)
  ret void
}

define void @stdlib.assert_fail(ptr %msg) {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %call = call i32 @puts(ptr @.str.9)
  %arg = load ptr, ptr %msg1, align 8
  %call2 = call i32 @puts(ptr %arg)
  call void @exit(i32 1)
  ret void
}

define void @stdlib.assert_true(i32 %cond, ptr %msg) {
entry:
  %cond1 = alloca i32, align 4
  store i32 %cond, ptr %cond1, align 4
  %msg2 = alloca ptr, align 8
  store ptr %msg, ptr %msg2, align 8
  %load = load i32, ptr %cond1, align 4
  %cond3 = icmp ne i32 %load, 0
  br i1 %cond3, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret void

if.end:                                           ; preds = %entry
  %arg = load ptr, ptr %msg2, align 8
  call void @stdlib.assert_fail(ptr %arg)
  ret void
}

define void @stdlib.assert_false(i32 %cond, ptr %msg) {
entry:
  %cond1 = alloca i32, align 4
  store i32 %cond, ptr %cond1, align 4
  %msg2 = alloca ptr, align 8
  store ptr %msg, ptr %msg2, align 8
  %load = load i32, ptr %cond1, align 4
  %cond3 = icmp ne i32 %load, 0
  %not = icmp eq i1 %cond3, false
  %not_ext = zext i1 %not to i32
  %cond4 = icmp ne i32 %not_ext, 0
  br i1 %cond4, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret void

if.end:                                           ; preds = %entry
  %arg = load ptr, ptr %msg2, align 8
  call void @stdlib.assert_fail(ptr %arg)
  ret void
}
