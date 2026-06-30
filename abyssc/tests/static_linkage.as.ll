; ModuleID = '/home/xator/Abyss/abyssc/tests/static_linkage.as'
source_filename = "/home/xator/Abyss/abyssc/tests/static_linkage.as"

@seed = internal global i32 9

define internal i32 @add_seed(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %cur = load i32, ptr @seed, align 4
  %load = load i32, ptr %value1, align 4
  %add = add i32 %cur, %load
  store i32 %add, ptr @seed, align 4
  %load2 = load i32, ptr @seed, align 4
  ret i32 %load2
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @add_seed(i32 4)
  %load = load i32, ptr @seed, align 4
  %add = add i32 %call, %load
  ret i32 %add
}
