; ModuleID = '/home/xator/Abyss/abyssc/tests/global_vars.as'
source_filename = "/home/xator/Abyss/abyssc/tests/global_vars.as"

@total = global i32 4
@scratch = global i32 0

define i32 @read_total(i32 %add) {
entry:
  %add1 = alloca i32, align 4
  store i32 %add, ptr %add1, align 4
  %load = load i32, ptr @total, align 4
  %load2 = load i32, ptr %add1, align 4
  %add3 = add i32 %load, %load2
  ret i32 %add3
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 6, ptr @scratch, align 4
  %cur = load i32, ptr @total, align 4
  %load = load i32, ptr @scratch, align 4
  %add = add i32 %cur, %load
  store i32 %add, ptr @total, align 4
  %call = call i32 @read_total(i32 2)
  %load3 = load i32, ptr @total, align 4
  %add4 = add i32 %call, %load3
  %load5 = load i32, ptr @scratch, align 4
  %add6 = add i32 %add4, %load5
  ret i32 %add6
}
