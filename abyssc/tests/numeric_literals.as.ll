; ModuleID = '/home/xator/Abyss/abyssc/tests/numeric_literals.as'
source_filename = "/home/xator/Abyss/abyssc/tests/numeric_literals.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %value = alloca double, align 8
  %octal = alloca i32, align 4
  %hex = alloca i32, align 4
  %binary = alloca i32, align 4
  %decimal = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 1000, ptr %decimal, align 4
  store i32 10, ptr %binary, align 4
  store i32 15, ptr %hex, align 4
  store i32 8, ptr %octal, align 4
  store double 2.500000e+01, ptr %value, align 8
  %load = load i32, ptr %decimal, align 4
  %div = sdiv i32 %load, 100
  %add = add i32 33, %div
  %load3 = load i32, ptr %binary, align 4
  %add4 = add i32 %add, %load3
  %load5 = load i32, ptr %hex, align 4
  %add6 = add i32 %add4, %load5
  %load7 = load i32, ptr %octal, align 4
  %add8 = add i32 %add6, %load7
  %load9 = load double, ptr %value, align 8
  %fdiv = fdiv double %load9, 5.000000e+00
  %fptosi = fptosi double %fdiv to i32
  %add10 = add i32 %add8, %fptosi
  ret i32 %add10
}
