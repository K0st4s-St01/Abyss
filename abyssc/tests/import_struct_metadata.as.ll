; ModuleID = '/home/xator/Abyss/abyssc/tests/import_struct_metadata.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_struct_metadata.as"

%Point = type { i32, i32 }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %p = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Point zeroinitializer, ptr %p, align 4
  %struct.init = getelementptr %Point, ptr %p, i32 0, i32 0
  store i32 6, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %p, i32 0, i32 1
  store i32 7, ptr %struct.init3, align 4
  %x = getelementptr %Point, ptr %p, i32 0, i32 0
  %y = getelementptr %Point, ptr %p, i32 0, i32 1
  %load = load i32, ptr %x, align 4
  %load4 = load i32, ptr %y, align 4
  %add = add i32 %load, %load4
  ret i32 %add
}
