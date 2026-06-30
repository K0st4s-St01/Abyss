; ModuleID = '/home/xator/Abyss/abyssc/tests/enum_alias.as'
source_filename = "/home/xator/Abyss/abyssc/tests/enum_alias.as"

define i32 @value(i32 %color) {
entry:
  %color1 = alloca i32, align 4
  store i32 %color, ptr %color1, align 4
  %load = load i32, ptr %color1, align 4
  br label %switch.cmp

switch.end:                                       ; No predecessors!
  ret i32 0

switch.default:                                   ; preds = %switch.cmp5
  ret i32 9

switch.case:                                      ; preds = %switch.cmp
  ret i32 1

switch.cmp:                                       ; preds = %entry
  %switch.eq = icmp eq i32 %load, 0
  br i1 %switch.eq, label %switch.case, label %switch.cmp3

switch.case2:                                     ; preds = %switch.cmp3
  ret i32 4

switch.cmp3:                                      ; preds = %switch.cmp
  %switch.eq6 = icmp eq i32 %load, 4
  br i1 %switch.eq6, label %switch.case2, label %switch.cmp5

switch.case4:                                     ; preds = %switch.cmp5
  ret i32 5

switch.cmp5:                                      ; preds = %switch.cmp3
  %switch.eq7 = icmp eq i32 %load, 5
  br i1 %switch.eq7, label %switch.case4, label %switch.default
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %total = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @value(i32 5)
  %add = add i32 %call, 4
  store i32 %add, ptr %total, align 4
  %load = load i32, ptr %total, align 4
  ret i32 %load
}
