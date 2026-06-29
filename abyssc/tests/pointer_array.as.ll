; ModuleID = '/home/xator/Abyss/abyssc/tests/pointer_array.as'
source_filename = "/home/xator/Abyss/abyssc/tests/pointer_array.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %result = alloca i32, align 4
  %xs = alloca ptr, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %malloc = call ptr @malloc(i64 12)
  store ptr %malloc, ptr %xs, align 8
  %deref = load ptr, ptr %xs, align 8
  %idx = getelementptr i32, ptr %deref, i32 0
  %deref3 = load ptr, ptr %xs, align 8
  %idx4 = getelementptr i32, ptr %deref3, i32 0
  store i32 10, ptr %idx4, align 4
  %deref5 = load ptr, ptr %xs, align 8
  %idx6 = getelementptr i32, ptr %deref5, i32 1
  %deref7 = load ptr, ptr %xs, align 8
  %idx8 = getelementptr i32, ptr %deref7, i32 1
  store i32 20, ptr %idx8, align 4
  %deref9 = load ptr, ptr %xs, align 8
  %idx10 = getelementptr i32, ptr %deref9, i32 2
  %deref11 = load ptr, ptr %xs, align 8
  %idx12 = getelementptr i32, ptr %deref11, i32 2
  store i32 30, ptr %idx12, align 4
  %deref13 = load ptr, ptr %xs, align 8
  %idx14 = getelementptr i32, ptr %deref13, i32 0
  %deref15 = load ptr, ptr %xs, align 8
  %idx16 = getelementptr i32, ptr %deref15, i32 1
  %load = load i32, ptr %idx14, align 4
  %load17 = load i32, ptr %idx16, align 4
  %add = add i32 %load, %load17
  %deref18 = load ptr, ptr %xs, align 8
  %idx19 = getelementptr i32, ptr %deref18, i32 2
  %load20 = load i32, ptr %idx19, align 4
  %add21 = add i32 %add, %load20
  store i32 %add21, ptr %result, align 4
  %del_ptr = load ptr, ptr %xs, align 8
  call void @free(ptr %del_ptr)
  %load22 = load i32, ptr %result, align 4
  ret i32 %load22
}

declare ptr @malloc(i64)

declare void @free(ptr)
