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

declare i32 @atoi(ptr)

declare i64 @atol(ptr)

declare double @atof(ptr)

declare i32 @abs(i32)

declare i64 @llabs(i64)

declare i32 @rand()

declare void @srand(i32)

declare void @exit(i32)

define i64 @str_len(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i64 @strlen(ptr %arg)
  ret i64 %call
}

define i32 @str_cmp(ptr %a, ptr %b) {
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

define i32 @str_cmp_n(ptr %a, ptr %b, i64 %n) {
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

define i32 @str_eq(ptr %a, ptr %b) {
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

define i32 @str_eq_n(ptr %a, ptr %b, i64 %n) {
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

define i32 @str_ne(ptr %a, ptr %b) {
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

define i32 @str_is_empty(ptr %s) {
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

define i32 @str_not_empty(ptr %s) {
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

define ptr @str_find_char(ptr %s, i32 %c) {
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

define ptr @str_find(ptr %haystack, ptr %needle) {
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

define i32 @str_to_i32(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @atoi(ptr %arg)
  ret i32 %call
}

define i64 @str_to_i64(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i64 @atol(ptr %arg)
  ret i64 %call
}

define double @str_to_f64(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call double @atof(ptr %arg)
  ret double %call
}

define ptr @mem_alloc(i64 %size) {
entry:
  %size1 = alloca i64, align 8
  store i64 %size, ptr %size1, align 4
  %arg = load i64, ptr %size1, align 4
  %call = call ptr @malloc(i64 %arg)
  ret ptr %call
}

define ptr @mem_alloc_zero(i64 %count, i64 %size) {
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

define ptr @mem_resize(ptr %ptr, i64 %size) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %size2 = alloca i64, align 8
  store i64 %size, ptr %size2, align 4
  %arg = load i8, ptr %ptr1, align 1
  %arg3 = load i64, ptr %size2, align 4
  %call = call ptr @realloc(i8 %arg, i64 %arg3)
  ret ptr %call
}

define void @mem_free(ptr %ptr) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %arg = load i8, ptr %ptr1, align 1
  call void @free(i8 %arg)
  ret void
}

define ptr @mem_copy(ptr %dst, ptr %src, i64 %n) {
entry:
  %dst1 = alloca ptr, align 8
  store ptr %dst, ptr %dst1, align 8
  %src2 = alloca ptr, align 8
  store ptr %src, ptr %src2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load i8, ptr %dst1, align 1
  %arg4 = load i8, ptr %src2, align 1
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memcpy(i8 %arg, i8 %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @mem_move(ptr %dst, ptr %src, i64 %n) {
entry:
  %dst1 = alloca ptr, align 8
  store ptr %dst, ptr %dst1, align 8
  %src2 = alloca ptr, align 8
  store ptr %src, ptr %src2, align 8
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load i8, ptr %dst1, align 1
  %arg4 = load i8, ptr %src2, align 1
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memmove(i8 %arg, i8 %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @mem_set(ptr %ptr, i32 %val, i64 %n) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %val2 = alloca i32, align 4
  store i32 %val, ptr %val2, align 4
  %n3 = alloca i64, align 8
  store i64 %n, ptr %n3, align 4
  %arg = load i8, ptr %ptr1, align 1
  %arg4 = load i32, ptr %val2, align 4
  %arg5 = load i64, ptr %n3, align 4
  %call = call ptr @memset(i8 %arg, i32 %arg4, i64 %arg5)
  ret ptr %call
}

define ptr @mem_zero(ptr %ptr, i64 %n) {
entry:
  %ptr1 = alloca ptr, align 8
  store ptr %ptr, ptr %ptr1, align 8
  %n2 = alloca i64, align 8
  store i64 %n, ptr %n2, align 4
  %arg = load i8, ptr %ptr1, align 1
  %arg3 = load i64, ptr %n2, align 4
  %call = call ptr @memset(i8 %arg, i32 0, i64 %arg3)
  ret ptr %call
}

define i32 @math_abs(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %arg = load i32, ptr %x1, align 4
  %call = call i32 @abs(i32 %arg)
  ret i32 %call
}

define i64 @math_abs_i64(i64 %x) {
entry:
  %x1 = alloca i64, align 8
  store i64 %x, ptr %x1, align 4
  %arg = load i64, ptr %x1, align 4
  %call = call i64 @llabs(i64 %arg)
  ret i64 %call
}

define i32 @math_min(i32 %a, i32 %b) {
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

define i32 @math_max(i32 %a, i32 %b) {
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

define i32 @math_clamp(i32 %val, i32 %lo, i32 %hi) {
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

define i32 @math_sign(i32 %x) {
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

define i64 @math_min_i64(i64 %a, i64 %b) {
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

define i64 @math_max_i64(i64 %a, i64 %b) {
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

define i64 @math_clamp_i64(i64 %val, i64 %lo, i64 %hi) {
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

define i32 @bool_not(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %not = icmp eq i32 %load, 0
  %not_ext = zext i1 %not to i32
  %cond = icmp ne i32 %not_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 1

if.end:                                           ; preds = %entry
  ret i32 0
}

define i32 @bool_and(i32 %a, i32 %b) {
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

define i32 @bool_or(i32 %a, i32 %b) {
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

define void @print(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 (ptr, ...) @printf(ptr @.str.2, ptr %arg)
  ret void
}

define void @println(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 @puts(ptr %arg)
  ret void
}

define void @print_char(i32 %c) {
entry:
  %c1 = alloca i32, align 4
  store i32 %c, ptr %c1, align 4
  %arg = load i32, ptr %c1, align 4
  %call = call i32 @putchar(i32 %arg)
  ret void
}

define void @print_i32(i32 %n) {
entry:
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %arg = load i32, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.3, i32 %arg)
  ret void
}

define void @print_i32_raw(i32 %n) {
entry:
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  %arg = load i32, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.4, i32 %arg)
  ret void
}

define void @print_i64(i64 %n) {
entry:
  %n1 = alloca i64, align 8
  store i64 %n, ptr %n1, align 4
  %arg = load i64, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.5, i64 %arg)
  ret void
}

define void @print_i64_raw(i64 %n) {
entry:
  %n1 = alloca i64, align 8
  store i64 %n, ptr %n1, align 4
  %arg = load i64, ptr %n1, align 4
  %call = call i32 (ptr, ...) @printf(ptr @.str.6, i64 %arg)
  ret void
}

define void @print_str(ptr %s) {
entry:
  %s1 = alloca ptr, align 8
  store ptr %s, ptr %s1, align 8
  %arg = load ptr, ptr %s1, align 8
  %call = call i32 (ptr, ...) @printf(ptr @.str.7, ptr %arg)
  ret void
}

define void @print_ptr(ptr %p) {
entry:
  %p1 = alloca ptr, align 8
  store ptr %p, ptr %p1, align 8
  %arg = load i8, ptr %p1, align 1
  %call = call i32 (ptr, ...) @printf(ptr @.str.8, i8 %arg)
  ret void
}

define i32 @input_char() {
entry:
  %call = call i32 @getchar()
  ret i32 %call
}

define i32 @random_i32() {
entry:
  %call = call i32 @rand()
  ret i32 %call
}

define void @random_seed(i32 %seed) {
entry:
  %seed1 = alloca i32, align 4
  store i32 %seed, ptr %seed1, align 4
  %arg = load i32, ptr %seed1, align 4
  call void @srand(i32 %arg)
  ret void
}

define void @process_exit(i32 %code) {
entry:
  %code1 = alloca i32, align 4
  store i32 %code, ptr %code1, align 4
  %arg = load i32, ptr %code1, align 4
  call void @exit(i32 %arg)
  ret void
}

define void @assert_fail(ptr %msg) {
entry:
  %msg1 = alloca ptr, align 8
  store ptr %msg, ptr %msg1, align 8
  %call = call i32 @puts(ptr @.str.9)
  %arg = load ptr, ptr %msg1, align 8
  %call2 = call i32 @puts(ptr %arg)
  call void @exit(i32 1)
  ret void
}

define void @assert_true(i32 %cond, ptr %msg) {
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
  call void @assert_fail(ptr %arg)
  ret void
}

define void @assert_false(i32 %cond, ptr %msg) {
entry:
  %cond1 = alloca i32, align 4
  store i32 %cond, ptr %cond1, align 4
  %msg2 = alloca ptr, align 8
  store ptr %msg, ptr %msg2, align 8
  %load = load i32, ptr %cond1, align 4
  %not = icmp eq i32 %load, 0
  %not_ext = zext i1 %not to i32
  %cond3 = icmp ne i32 %not_ext, 0
  br i1 %cond3, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret void

if.end:                                           ; preds = %entry
  %arg = load ptr, ptr %msg2, align 8
  call void @assert_fail(ptr %arg)
  ret void
}
