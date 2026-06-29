; ModuleID = '/home/xator/Abyss/abyssc/tests/high_priority_literals.as'
source_filename = "/home/xator/Abyss/abyssc/tests/high_priority_literals.as"

define i64 @pick_i64(i64 %value) {
entry:
  %value1 = alloca i64, align 8
  store i64 %value, ptr %value1, align 4
  %load = load i64, ptr %value1, align 4
  %eq = icmp eq i64 %load, 3
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load2 = load i64, ptr %value1, align 4
  %add = add i64 %load2, 4
  ret i64 %add

if.end:                                           ; preds = %entry
  ret i64 0
}

define ptr @none() {
entry:
  ret ptr null
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %result = alloca i64, align 8
  %p = alloca ptr, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store ptr null, ptr %p, align 8
  %call = call ptr @none()
  %eq = icmp eq ptr %call, null
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load = load ptr, ptr %p, align 8
  %eq3 = icmp eq ptr %load, null
  %eq_ext4 = zext i1 %eq3 to i32
  %cond5 = icmp ne i32 %eq_ext4, 0
  br i1 %cond5, label %if.then6, label %if.end7

if.end:                                           ; preds = %if.end7, %entry
  ret i32 1

if.then6:                                         ; preds = %if.then
  %call8 = call i64 @pick_i64(i64 3)
  store i64 %call8, ptr %result, align 4
  %load9 = load i64, ptr %result, align 4
  %eq10 = icmp eq i64 %load9, 7
  %eq_ext11 = zext i1 %eq10 to i32
  %cond12 = icmp ne i32 %eq_ext11, 0
  br i1 %cond12, label %if.then13, label %if.end14

if.end7:                                          ; preds = %if.end14, %if.then
  br label %if.end

if.then13:                                        ; preds = %if.then6
  ret i32 42

if.end14:                                         ; preds = %if.then6
  br label %if.end7
}
