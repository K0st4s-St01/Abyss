; ModuleID = '/home/xator/Abyss/abyssc/tests/extern_globals.as'
source_filename = "/home/xator/Abyss/abyssc/tests/extern_globals.as"

@external_counter = external global i32

define i32 @read_external() {
entry:
  %load = load i32, ptr @external_counter, align 4
  ret i32 %load
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  ret i32 0
}
