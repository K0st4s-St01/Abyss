; ModuleID = '/home/xator/Abyss/abyssc/tests/string_conditions.as'
source_filename = "/home/xator/Abyss/abyssc/tests/string_conditions.as"

@.str = private constant [6 x i8] c"abyss\00"

define ptr @empty_ptr() {
entry:
  ret ptr null
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %total = alloca i32, align 4
  %missing = alloca ptr, align 8
  %s = alloca ptr, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store ptr @.str, ptr %s, align 8
  %call = call ptr @empty_ptr()
  store ptr %call, ptr %missing, align 8
  store i32 0, ptr %total, align 4
  %load = load ptr, ptr %s, align 8
  %cond = icmp ne ptr %load, null
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cur = load i32, ptr %total, align 4
  %add = add i32 %cur, 10
  store i32 %add, ptr %total, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %load3 = load ptr, ptr %missing, align 8
  %cond4 = icmp ne ptr %load3, null
  %not = icmp eq i1 %cond4, false
  %not_ext = zext i1 %not to i32
  %cond5 = icmp ne i32 %not_ext, 0
  br i1 %cond5, label %if.then6, label %if.end7

if.then6:                                         ; preds = %if.end
  %cur8 = load i32, ptr %total, align 4
  %add9 = add i32 %cur8, 11
  store i32 %add9, ptr %total, align 4
  br label %if.end7

if.end7:                                          ; preds = %if.then6, %if.end
  %load10 = load ptr, ptr %missing, align 8
  %eq = icmp eq ptr %load10, null
  %eq_ext = zext i1 %eq to i32
  %cond11 = icmp ne i32 %eq_ext, 0
  br i1 %cond11, label %if.then12, label %if.end13

if.then12:                                        ; preds = %if.end7
  %cur14 = load i32, ptr %total, align 4
  %add15 = add i32 %cur14, 12
  store i32 %add15, ptr %total, align 4
  br label %if.end13

if.end13:                                         ; preds = %if.then12, %if.end7
  %load16 = load ptr, ptr %s, align 8
  %ne = icmp ne ptr %load16, null
  %ne_ext = zext i1 %ne to i32
  %cond17 = icmp ne i32 %ne_ext, 0
  br i1 %cond17, label %if.then18, label %if.end19

if.then18:                                        ; preds = %if.end13
  %cur20 = load i32, ptr %total, align 4
  %add21 = add i32 %cur20, 9
  store i32 %add21, ptr %total, align 4
  br label %if.end19

if.end19:                                         ; preds = %if.then18, %if.end13
  %load22 = load i32, ptr %total, align 4
  ret i32 %load22
}
