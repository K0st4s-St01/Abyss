; ModuleID = '/home/xator/Abyss/abyssc/tests/conditional_expr.as'
source_filename = "/home/xator/Abyss/abyssc/tests/conditional_expr.as"

define i32 @choose(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %sgt = icmp sgt i32 %load, 10
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %cond.then, label %cond.else

cond.then:                                        ; preds = %entry
  %load2 = load i32, ptr %value1, align 4
  br label %cond.end

cond.else:                                        ; preds = %entry
  br label %cond.end

cond.end:                                         ; preds = %cond.else, %cond.then
  %cond3 = phi i32 [ %load2, %cond.then ], [ 10, %cond.else ]
  ret i32 %cond3
}

define i32 @nested(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %slt = icmp slt i32 %load, 0
  %slt_ext = zext i1 %slt to i32
  %cond = icmp ne i32 %slt_ext, 0
  br i1 %cond, label %cond.then, label %cond.else

cond.then:                                        ; preds = %entry
  br label %cond.end

cond.else:                                        ; preds = %entry
  %load2 = load i32, ptr %value1, align 4
  %eq = icmp eq i32 %load2, 0
  %eq_ext = zext i1 %eq to i32
  %cond3 = icmp ne i32 %eq_ext, 0
  br i1 %cond3, label %cond.then4, label %cond.else5

cond.end:                                         ; preds = %cond.end6, %cond.then
  %cond8 = phi i32 [ 1, %cond.then ], [ %cond7, %cond.end6 ]
  ret i32 %cond8

cond.then4:                                       ; preds = %cond.else
  br label %cond.end6

cond.else5:                                       ; preds = %cond.else
  br label %cond.end6

cond.end6:                                        ; preds = %cond.else5, %cond.then4
  %cond7 = phi i32 [ 2, %cond.then4 ], [ 3, %cond.else5 ]
  br label %cond.end
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %e = alloca i32, align 4
  %d = alloca i32, align 4
  %c = alloca i32, align 4
  %b = alloca i32, align 4
  %a = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @choose(i32 4)
  store i32 %call, ptr %a, align 4
  %call3 = call i32 @choose(i32 15)
  store i32 %call3, ptr %b, align 4
  %call4 = call i32 @nested(i32 -1)
  store i32 %call4, ptr %c, align 4
  %call5 = call i32 @nested(i32 0)
  store i32 %call5, ptr %d, align 4
  %call6 = call i32 @nested(i32 8)
  store i32 %call6, ptr %e, align 4
  %load = load i32, ptr %a, align 4
  %load7 = load i32, ptr %b, align 4
  %add = add i32 %load, %load7
  %load8 = load i32, ptr %c, align 4
  %add9 = add i32 %add, %load8
  %load10 = load i32, ptr %d, align 4
  %add11 = add i32 %add9, %load10
  %load12 = load i32, ptr %e, align 4
  %add13 = add i32 %add11, %load12
  ret i32 %add13
}
