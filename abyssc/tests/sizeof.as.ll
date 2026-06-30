; ModuleID = '/home/xator/Abyss/abyssc/tests/sizeof.as'
source_filename = "/home/xator/Abyss/abyssc/tests/sizeof.as"

@global_size_source = global i64 0

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %local_size_source = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %local_size_source, align 4
  ret i32 trunc (i64 add (i64 add (i64 add (i64 add (i64 ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64), i64 ptrtoint (ptr getelementptr (i16, ptr null, i32 1) to i64)), i64 ptrtoint (ptr getelementptr (ptr, ptr null, i32 1) to i64)), i64 ptrtoint (ptr getelementptr (i64, ptr null, i32 1) to i64)), i64 ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64)) to i32)
}
