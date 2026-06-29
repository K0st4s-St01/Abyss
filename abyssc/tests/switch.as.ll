; ModuleID = '/home/xator/Abyss/abyssc/tests/switch.as'
source_filename = "/home/xator/Abyss/abyssc/tests/switch.as"

define i32 @classify(i32 %x) {
entry:
  %result = alloca i32, align 4
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  store i32 0, ptr %result, align 4
  %load = load i32, ptr %x1, align 4
  br label %switch.cmp

switch.end:                                       ; preds = %switch.default, %switch.case2, %switch.case
  %load8 = load i32, ptr %result, align 4
  ret i32 %load8

switch.default:                                   ; preds = %switch.cmp5
  store i32 40, ptr %result, align 4
  br label %switch.end

switch.case:                                      ; preds = %switch.cmp
  store i32 10, ptr %result, align 4
  br label %switch.end

switch.cmp:                                       ; preds = %entry
  %switch.eq = icmp eq i32 %load, 1
  br i1 %switch.eq, label %switch.case, label %switch.cmp3

switch.case2:                                     ; preds = %switch.cmp3
  store i32 20, ptr %result, align 4
  br label %switch.end

switch.cmp3:                                      ; preds = %switch.cmp
  %switch.eq6 = icmp eq i32 %load, 2
  br i1 %switch.eq6, label %switch.case2, label %switch.cmp5

switch.case4:                                     ; preds = %switch.cmp5
  ret i32 30

switch.cmp5:                                      ; preds = %switch.cmp3
  %switch.eq7 = icmp eq i32 %load, 3
  br i1 %switch.eq7, label %switch.case4, label %switch.default
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %c = alloca i32, align 4
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @classify(i32 2)
  store i32 %call, ptr %a, align 4
  %call3 = call i32 @classify(i32 3)
  store i32 %call3, ptr %b, align 4
  %call4 = call i32 @classify(i32 9)
  store i32 %call4, ptr %c, align 4
  %load = load i32, ptr %a, align 4
  %load5 = load i32, ptr %b, align 4
  %add = add i32 %load, %load5
  %load6 = load i32, ptr %c, align 4
  %add7 = add i32 %add, %load6
  ret i32 %add7
}
