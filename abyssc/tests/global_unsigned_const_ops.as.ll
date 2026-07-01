; ModuleID = '/home/xator/Abyss/abyssc/tests/global_unsigned_const_ops.as'
source_filename = "/home/xator/Abyss/abyssc/tests/global_unsigned_const_ops.as"

@half64 = global i64 9223372036854775807
@rem64 = global i64 1
@shifted64 = global i64 1
@half32 = global i32 2147483647
@rem32 = global i32 1
@shifted32 = global i32 1

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %total = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %total, align 4
  %load = load i64, ptr @half64, align 4
  %sgt = icmp ugt i64 %load, 1
  %sgt_ext = zext i1 %sgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cur = load i32, ptr %total, align 4
  %add = add i32 %cur, 10
  store i32 %add, ptr %total, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %load3 = load i64, ptr @rem64, align 4
  %eq = icmp eq i64 %load3, 1
  %eq_ext = zext i1 %eq to i32
  %cond4 = icmp ne i32 %eq_ext, 0
  br i1 %cond4, label %if.then5, label %if.end6

if.then5:                                         ; preds = %if.end
  %cur7 = load i32, ptr %total, align 4
  %add8 = add i32 %cur7, 2
  store i32 %add8, ptr %total, align 4
  br label %if.end6

if.end6:                                          ; preds = %if.then5, %if.end
  %load9 = load i64, ptr @shifted64, align 4
  %eq10 = icmp eq i64 %load9, 1
  %eq_ext11 = zext i1 %eq10 to i32
  %cond12 = icmp ne i32 %eq_ext11, 0
  br i1 %cond12, label %if.then13, label %if.end14

if.then13:                                        ; preds = %if.end6
  %cur15 = load i32, ptr %total, align 4
  %add16 = add i32 %cur15, 3
  store i32 %add16, ptr %total, align 4
  br label %if.end14

if.end14:                                         ; preds = %if.then13, %if.end6
  %load17 = load i32, ptr @half32, align 4
  %sgt18 = icmp ugt i32 %load17, 1
  %sgt_ext19 = zext i1 %sgt18 to i32
  %cond20 = icmp ne i32 %sgt_ext19, 0
  br i1 %cond20, label %if.then21, label %if.end22

if.then21:                                        ; preds = %if.end14
  %cur23 = load i32, ptr %total, align 4
  %add24 = add i32 %cur23, 4
  store i32 %add24, ptr %total, align 4
  br label %if.end22

if.end22:                                         ; preds = %if.then21, %if.end14
  %load25 = load i32, ptr @rem32, align 4
  %eq26 = icmp eq i32 %load25, 1
  %eq_ext27 = zext i1 %eq26 to i32
  %cond28 = icmp ne i32 %eq_ext27, 0
  br i1 %cond28, label %if.then29, label %if.end30

if.then29:                                        ; preds = %if.end22
  %cur31 = load i32, ptr %total, align 4
  %add32 = add i32 %cur31, 5
  store i32 %add32, ptr %total, align 4
  br label %if.end30

if.end30:                                         ; preds = %if.then29, %if.end22
  %load33 = load i32, ptr @shifted32, align 4
  %eq34 = icmp eq i32 %load33, 1
  %eq_ext35 = zext i1 %eq34 to i32
  %cond36 = icmp ne i32 %eq_ext35, 0
  br i1 %cond36, label %if.then37, label %if.end38

if.then37:                                        ; preds = %if.end30
  %cur39 = load i32, ptr %total, align 4
  %add40 = add i32 %cur39, 6
  store i32 %add40, ptr %total, align 4
  br label %if.end38

if.end38:                                         ; preds = %if.then37, %if.end30
  %load41 = load i32, ptr %total, align 4
  ret i32 %load41
}
