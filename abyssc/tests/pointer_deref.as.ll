; ModuleID = '/home/xator/Abyss/abyssc/tests/pointer_deref.as'
source_filename = "/home/xator/Abyss/abyssc/tests/pointer_deref.as"

define i32 @store(ptr %slot) {
entry:
  %slot1 = alloca ptr, align 8
  store ptr %slot, ptr %slot1, align 8
  %load = load ptr, ptr %slot1, align 8
  %load2 = load ptr, ptr %slot1, align 8
  store i32 7, ptr %load2, align 4
  ret i32 0
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %value = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %value, align 4
  %call = call i32 @store(ptr %value)
  %load = load i32, ptr %value, align 4
  ret i32 %load
}
