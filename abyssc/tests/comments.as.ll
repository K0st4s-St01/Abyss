; ModuleID = '/home/xator/Abyss/abyssc/tests/comments.as'
source_filename = "/home/xator/Abyss/abyssc/tests/comments.as"

@value = internal global i32 3

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %cur = load i32, ptr @value, align 4
  %add = add i32 %cur, 4
  store i32 %add, ptr @value, align 4
  %load = load i32, ptr @value, align 4
  ret i32 %load
}
