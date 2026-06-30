; ModuleID = '/home/xator/Abyss/abyssc/tests/logical_short_circuit.as'
source_filename = "/home/xator/Abyss/abyssc/tests/logical_short_circuit.as"

define i32 @mark(ptr %slot) {
entry:
  %slot1 = alloca ptr, align 8
  store ptr %slot, ptr %slot1, align 8
  %load = load ptr, ptr %slot1, align 8
  %load2 = load ptr, ptr %slot1, align 8
  store i32 1, ptr %load2, align 4
  ret i32 1
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %hit = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %hit, align 4
  br i1 false, label %logic.rhs, label %logic.end

logic.rhs:                                        ; preds = %entry
  %call = call i32 @mark(ptr %hit)
  %cond = icmp ne i32 %call, 0
  br label %logic.end

logic.end:                                        ; preds = %logic.rhs, %entry
  %land = phi i1 [ false, %entry ], [ %cond, %logic.rhs ]
  %logic_ext = zext i1 %land to i32
  %cond3 = icmp ne i32 %logic_ext, 0
  br i1 %cond3, label %if.then, label %if.end

if.then:                                          ; preds = %logic.end
  ret i32 1

if.end:                                           ; preds = %logic.end
  %load = load i32, ptr %hit, align 4
  %ne = icmp ne i32 %load, 0
  %ne_ext = zext i1 %ne to i32
  %cond4 = icmp ne i32 %ne_ext, 0
  br i1 %cond4, label %if.then5, label %if.end6

if.then5:                                         ; preds = %if.end
  ret i32 2

if.end6:                                          ; preds = %if.end
  br i1 true, label %logic.end8, label %logic.rhs7

logic.rhs7:                                       ; preds = %if.end6
  %call9 = call i32 @mark(ptr %hit)
  %cond10 = icmp ne i32 %call9, 0
  br label %logic.end8

logic.end8:                                       ; preds = %logic.rhs7, %if.end6
  %lor = phi i1 [ true, %if.end6 ], [ %cond10, %logic.rhs7 ]
  %logic_ext11 = zext i1 %lor to i32
  %cond12 = icmp ne i32 %logic_ext11, 0
  br i1 %cond12, label %if.then13, label %if.end14

if.then13:                                        ; preds = %logic.end8
  %load15 = load i32, ptr %hit, align 4
  %eq = icmp eq i32 %load15, 0
  %eq_ext = zext i1 %eq to i32
  %cond16 = icmp ne i32 %eq_ext, 0
  br i1 %cond16, label %if.then17, label %if.end18

if.end14:                                         ; preds = %logic.end8
  ret i32 4

if.then17:                                        ; preds = %if.then13
  ret i32 0

if.end18:                                         ; preds = %if.then13
  ret i32 3
}
