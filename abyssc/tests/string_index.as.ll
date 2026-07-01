; ModuleID = '/home/xator/Abyss/abyssc/tests/string_index.as'
source_filename = "/home/xator/Abyss/abyssc/tests/string_index.as"

@.str = private constant [6 x i8] c"abyss\00"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %value = alloca i32, align 4
  %s = alloca ptr, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store ptr @.str, ptr %s, align 8
  %deref = load ptr, ptr %s, align 8
  %idx = getelementptr i8, ptr %deref, i32 1
  %load = load i8, ptr %idx, align 1
  %sext = sext i8 %load to i32
  store i32 %sext, ptr %value, align 4
  %deref3 = load ptr, ptr %s, align 8
  %idx4 = getelementptr i8, ptr %deref3, i32 5
  %load5 = load i8, ptr %idx4, align 1
  %sext6 = sext i8 %load5 to i32
  %eq = icmp eq i32 %sext6, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load7 = load i32, ptr %value, align 4
  %sub = sub i32 %load7, 56
  ret i32 %sub

if.end:                                           ; preds = %entry
  ret i32 1
}
