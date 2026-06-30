; ModuleID = '/home/xator/Abyss/abyssc/tests/forward_calls.as'
source_filename = "/home/xator/Abyss/abyssc/tests/forward_calls.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %x = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @make()
  store i32 %call, ptr %x, align 4
  %call3 = call i32 @later(i32 4)
  %load = load i32, ptr %x, align 4
  %add = add i32 %call3, %load
  ret i32 %add
}

define i32 @make() {
entry:
  ret i32 14
}

define i32 @later(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %add = add i32 %load, 8
  ret i32 %add
}
