; ModuleID = '/home/xator/Abyss/abyssc/tests/increment_decrement.as'
source_filename = "/home/xator/Abyss/abyssc/tests/increment_decrement.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %j = alloca i32, align 4
  %total = alloca i32, align 4
  %i = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %i, align 4
  %cur = load i32, ptr %i, align 4
  %add = add i32 %cur, 1
  store i32 %add, ptr %i, align 4
  %cur3 = load i32, ptr %i, align 4
  %add4 = add i32 %cur3, 1
  store i32 %add4, ptr %i, align 4
  %cur5 = load i32, ptr %i, align 4
  %sub = sub i32 %cur5, 1
  store i32 %sub, ptr %i, align 4
  %cur6 = load i32, ptr %i, align 4
  %sub7 = sub i32 %cur6, 1
  store i32 %sub7, ptr %i, align 4
  store i32 0, ptr %total, align 4
  store i32 0, ptr %j, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %load = load i32, ptr %j, align 4
  %slt = icmp slt i32 %load, 3
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %cur8 = load i32, ptr %total, align 4
  %load9 = load i32, ptr %j, align 4
  %add10 = add i32 %cur8, %load9
  store i32 %add10, ptr %total, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %cur11 = load i32, ptr %j, align 4
  %add12 = add i32 %cur11, 1
  store i32 %add12, ptr %j, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %load13 = load i32, ptr %i, align 4
  %load14 = load i32, ptr %total, align 4
  %add15 = add i32 %load13, %load14
  ret i32 %add15
}
