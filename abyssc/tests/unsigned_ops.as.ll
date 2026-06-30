; ModuleID = '/home/xator/Abyss/abyssc/tests/unsigned_ops.as'
source_filename = "/home/xator/Abyss/abyssc/tests/unsigned_ops.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %total = alloca i32, align 4
  %shifted = alloca i32, align 4
  %rem = alloca i32, align 4
  %half = alloca i32, align 4
  %thirty_one = alloca i32, align 4
  %two = alloca i32, align 4
  %one = alloca i32, align 4
  %max = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 -1, ptr %max, align 4
  store i32 1, ptr %one, align 4
  store i32 2, ptr %two, align 4
  store i32 31, ptr %thirty_one, align 4
  %load = load i32, ptr %max, align 4
  %load3 = load i32, ptr %two, align 4
  %udiv = udiv i32 %load, %load3
  store i32 %udiv, ptr %half, align 4
  %load4 = load i32, ptr %max, align 4
  %load5 = load i32, ptr %two, align 4
  %urem = urem i32 %load4, %load5
  store i32 %urem, ptr %rem, align 4
  %load6 = load i32, ptr %max, align 4
  %load7 = load i32, ptr %thirty_one, align 4
  %lshr = lshr i32 %load6, %load7
  store i32 %lshr, ptr %shifted, align 4
  store i32 0, ptr %total, align 4
  %load8 = load i32, ptr %max, align 4
  %load9 = load i32, ptr %one, align 4
  %sgt = icmp ugt i32 %load8, %load9
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cur = load i32, ptr %total, align 4
  %add = add i32 %cur, 10
  store i32 %add, ptr %total, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %load10 = load i32, ptr %half, align 4
  %load11 = load i32, ptr %one, align 4
  %sgt12 = icmp ugt i32 %load10, %load11
  %sgt_ext13 = zext i1 %sgt12 to i32
  %cond14 = icmp ne i32 %sgt_ext13, 0
  br i1 %cond14, label %if.then15, label %if.end16

if.then15:                                        ; preds = %if.end
  %cur17 = load i32, ptr %total, align 4
  %add18 = add i32 %cur17, 20
  store i32 %add18, ptr %total, align 4
  br label %if.end16

if.end16:                                         ; preds = %if.then15, %if.end
  %load19 = load i32, ptr %rem, align 4
  %load20 = load i32, ptr %one, align 4
  %eq = icmp eq i32 %load19, %load20
  %eq_ext = zext i1 %eq to i32
  %cond21 = icmp ne i32 %eq_ext, 0
  br i1 %cond21, label %if.then22, label %if.end23

if.then22:                                        ; preds = %if.end16
  %cur24 = load i32, ptr %total, align 4
  %add25 = add i32 %cur24, 3
  store i32 %add25, ptr %total, align 4
  br label %if.end23

if.end23:                                         ; preds = %if.then22, %if.end16
  %load26 = load i32, ptr %shifted, align 4
  %load27 = load i32, ptr %one, align 4
  %eq28 = icmp eq i32 %load26, %load27
  %eq_ext29 = zext i1 %eq28 to i32
  %cond30 = icmp ne i32 %eq_ext29, 0
  br i1 %cond30, label %if.then31, label %if.end32

if.then31:                                        ; preds = %if.end23
  %cur33 = load i32, ptr %total, align 4
  %add34 = add i32 %cur33, 4
  store i32 %add34, ptr %total, align 4
  br label %if.end32

if.end32:                                         ; preds = %if.then31, %if.end23
  %load35 = load i32, ptr %total, align 4
  ret i32 %load35
}
