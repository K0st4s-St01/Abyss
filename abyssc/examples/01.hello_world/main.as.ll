; ModuleID = 'abyssc/examples/01.hello_world/main.as'
source_filename = "abyssc/examples/01.hello_world/main.as"

%Point = type { i32, i32 }

define i32 @distance_sq(ptr %self, %Point %other) {
entry:
  %dy = alloca i32, align 4
  %dx = alloca i32, align 4
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %other2 = alloca %Point, align 8
  store %Point %other, ptr %other2, align 4
  %deref = load ptr, ptr %self1, align 8
  %x = getelementptr %Point, ptr %deref, i32 0, i32 0
  %x3 = getelementptr %Point, ptr %other2, i32 0, i32 0
  %load = load i32, ptr %x, align 4
  %load4 = load i32, ptr %x3, align 4
  %sub = sub i32 %load, %load4
  store i32 %sub, ptr %dx, align 4
  %deref5 = load ptr, ptr %self1, align 8
  %y = getelementptr %Point, ptr %deref5, i32 0, i32 1
  %y6 = getelementptr %Point, ptr %other2, i32 0, i32 1
  %load7 = load i32, ptr %y, align 4
  %load8 = load i32, ptr %y6, align 4
  %sub9 = sub i32 %load7, %load8
  store i32 %sub9, ptr %dy, align 4
  %load10 = load i32, ptr %dx, align 4
  %load11 = load i32, ptr %dx, align 4
  %mul = mul i32 %load10, %load11
  %load12 = load i32, ptr %dy, align 4
  %load13 = load i32, ptr %dy, align 4
  %mul14 = mul i32 %load12, %load13
  %add = add i32 %mul, %mul14
  ret i32 %add
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  ret i32 0
}
