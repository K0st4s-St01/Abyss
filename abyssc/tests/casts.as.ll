; ModuleID = '/home/xator/Abyss/abyssc/tests/casts.as'
source_filename = "/home/xator/Abyss/abyssc/tests/casts.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %again = alloca double, align 8
  %whole = alloca i32, align 4
  %value = alloca double, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store double 1.275000e+01, ptr %value, align 8
  %load = load double, ptr %value, align 8
  %fptosi = fptosi double %load to i32
  store i32 %fptosi, ptr %whole, align 4
  %load3 = load i32, ptr %whole, align 4
  %sitofp = sitofp i32 %load3 to double
  %fadd = fadd double %sitofp, 1.500000e+00
  store double %fadd, ptr %again, align 8
  %load4 = load double, ptr %again, align 8
  %fgt = fcmp ogt double %load4, 1.300000e+01
  %sgt_ext = zext i1 %fgt to i32
  %cond = icmp ne i32 %sgt_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load5 = load i32, ptr %whole, align 4
  %add = add i32 %load5, 1
  ret i32 %add

if.end:                                           ; preds = %entry
  ret i32 1
}
