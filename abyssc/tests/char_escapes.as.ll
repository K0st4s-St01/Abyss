; ModuleID = '/home/xator/Abyss/abyssc/tests/char_escapes.as'
source_filename = "/home/xator/Abyss/abyssc/tests/char_escapes.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %zero = alloca i8, align 1
  %slash = alloca i8, align 1
  %quote = alloca i8, align 1
  %tab = alloca i8, align 1
  %newline = alloca i8, align 1
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i8 10, ptr %newline, align 1
  store i8 9, ptr %tab, align 1
  store i8 39, ptr %quote, align 1
  store i8 92, ptr %slash, align 1
  store i8 0, ptr %zero, align 1
  %load = load i8, ptr %newline, align 1
  %sext = sext i8 %load to i32
  %load3 = load i8, ptr %tab, align 1
  %sext4 = sext i8 %load3 to i32
  %add = add i32 %sext, %sext4
  %load5 = load i8, ptr %quote, align 1
  %sext6 = sext i8 %load5 to i32
  %add7 = add i32 %add, %sext6
  %load8 = load i8, ptr %slash, align 1
  %sext9 = sext i8 %load8 to i32
  %add10 = add i32 %add7, %sext9
  %load11 = load i8, ptr %zero, align 1
  %sext12 = sext i8 %load11 to i32
  %add13 = add i32 %add10, %sext12
  ret i32 %add13
}
