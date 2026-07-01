; ModuleID = '/home/xator/Abyss/abyssc/tests/global_const_expressions.as'
source_filename = "/home/xator/Abyss/abyssc/tests/global_const_expressions.as"

@arithmetic = global i32 14
@shifted = global i32 16
@masked = global i32 12
@inverted = global i32 0
@ratio = global double 4.500000e+00

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %load = load double, ptr @ratio, align 8
  %fgt = fcmp ogt double %load, 4.000000e+00
  %sgt_ext = zext i1 %fgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load3 = load i32, ptr @arithmetic, align 4
  %load4 = load i32, ptr @masked, align 4
  %add = add i32 %load3, %load4
  %load5 = load i32, ptr @inverted, align 4
  %add6 = add i32 %add, %load5
  ret i32 %add6

if.end:                                           ; preds = %entry
  ret i32 1
}
