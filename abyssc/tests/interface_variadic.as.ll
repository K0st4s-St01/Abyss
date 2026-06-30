; ModuleID = '/home/xator/Abyss/abyssc/tests/interface_variadic.as'
source_filename = "/home/xator/Abyss/abyssc/tests/interface_variadic.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  ret i32 0
}
