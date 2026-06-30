; ModuleID = '/home/xator/Abyss/abyssc/tests/struct_arrow.as'
source_filename = "/home/xator/Abyss/abyssc/tests/struct_arrow.as"

%Point = type { i32, i32 }

define i32 @sum(ptr %point) {
entry:
  %point1 = alloca ptr, align 8
  store ptr %point, ptr %point1, align 8
  %load = load ptr, ptr %point1, align 8
  %x = getelementptr %Point, ptr %load, i32 0, i32 0
  %load2 = load ptr, ptr %point1, align 8
  %y = getelementptr %Point, ptr %load2, i32 0, i32 1
  %load3 = load i32, ptr %x, align 4
  %load4 = load i32, ptr %y, align 4
  %add = add i32 %load3, %load4
  ret i32 %add
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %p = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %x = getelementptr %Point, ptr %p, i32 0, i32 0
  %x3 = getelementptr %Point, ptr %p, i32 0, i32 0
  store i32 2, ptr %x3, align 4
  %y = getelementptr %Point, ptr %p, i32 0, i32 1
  %y4 = getelementptr %Point, ptr %p, i32 0, i32 1
  store i32 5, ptr %y4, align 4
  %call = call i32 @sum(ptr %p)
  ret i32 %call
}
