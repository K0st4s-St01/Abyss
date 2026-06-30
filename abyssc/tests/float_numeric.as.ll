; ModuleID = '/home/xator/Abyss/abyssc/tests/float_numeric.as'
source_filename = "/home/xator/Abyss/abyssc/tests/float_numeric.as"

define double @scale(double %value) {
entry:
  %value1 = alloca double, align 8
  store double %value, ptr %value1, align 8
  %load = load double, ptr %value1, align 8
  %fmul = fmul double %load, 2.000000e+00
  ret double %fmul
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %mixed = alloca double, align 8
  %small = alloca float, align 4
  %total = alloca double, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call double @scale(double 1.500000e+00)
  store double %call, ptr %total, align 8
  %cur = load double, ptr %total, align 8
  %fadd = fadd double %cur, 1.000000e+00
  store double %fadd, ptr %total, align 8
  store float 2.500000e+00, ptr %small, align 4
  %load = load double, ptr %total, align 8
  %load3 = load float, ptr %small, align 4
  %fpext = fpext float %load3 to double
  %fadd4 = fadd double %load, %fpext
  store double %fadd4, ptr %mixed, align 8
  %load5 = load double, ptr %mixed, align 8
  %fge = fcmp oge double %load5, 6.500000e+00
  %sge_ext = zext i1 %fge to i32
  %cond = icmp ne i32 %sge_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load6 = load float, ptr %small, align 4
  %fneg = fneg float %load6
  %fpext7 = fpext float %fneg to double
  %flt = fcmp olt double %fpext7, 0.000000e+00
  %slt_ext = zext i1 %flt to i32
  %cond8 = icmp ne i32 %slt_ext, 0
  br i1 %cond8, label %if.then9, label %if.end10

if.end:                                           ; preds = %if.end10, %entry
  ret i32 1

if.then9:                                         ; preds = %if.then
  ret i32 42

if.end10:                                         ; preds = %if.then
  br label %if.end
}
