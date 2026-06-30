; ModuleID = '/home/xator/Abyss/abyssc/tests/bool_literals.as'
source_filename = "/home/xator/Abyss/abyssc/tests/bool_literals.as"

define i1 @invert(i1 %value) {
entry:
  %value1 = alloca i1, align 1
  store i1 %value, ptr %value1, align 1
  %load = load i1, ptr %value1, align 1
  %not = icmp eq i1 %load, false
  %not_ext = zext i1 %not to i32
  %0 = trunc i32 %not_ext to i1
  ret i1 %0
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %no = alloca i1, align 1
  %yes = alloca i1, align 1
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i1 true, ptr %yes, align 1
  store i1 false, ptr %no, align 1
  %load = load i1, ptr %yes, align 1
  br i1 %load, label %logic.rhs, label %logic.end

logic.rhs:                                        ; preds = %entry
  %arg = load i1, ptr %no, align 1
  %call = call i1 @invert(i1 %arg)
  br label %logic.end

logic.end:                                        ; preds = %logic.rhs, %entry
  %land = phi i1 [ false, %entry ], [ %call, %logic.rhs ]
  %logic_ext = zext i1 %land to i32
  %cond = icmp ne i32 %logic_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %logic.end
  ret i32 12

if.end:                                           ; preds = %logic.end
  ret i32 1
}
