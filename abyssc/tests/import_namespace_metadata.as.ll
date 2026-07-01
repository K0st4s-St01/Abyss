; ModuleID = '/home/xator/Abyss/abyssc/tests/import_namespace_metadata.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_namespace_metadata.as"

%right.Point = type { i32 }
%left.Point = type { i32 }

declare i32 @left.answer()

declare i32 @answer()

declare i32 @right.answer()

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %rp = alloca %right.Point, align 8
  %lp = alloca %left.Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %left.Point zeroinitializer, ptr %lp, align 4
  %struct.init = getelementptr %left.Point, ptr %lp, i32 0, i32 0
  store i32 1, ptr %struct.init, align 4
  store %right.Point zeroinitializer, ptr %rp, align 4
  %struct.init3 = getelementptr %right.Point, ptr %rp, i32 0, i32 0
  store i32 2, ptr %struct.init3, align 4
  %x = getelementptr %left.Point, ptr %lp, i32 0, i32 0
  %x4 = getelementptr %right.Point, ptr %rp, i32 0, i32 0
  %load = load i32, ptr %x, align 4
  %load5 = load i32, ptr %x4, align 4
  %add = add i32 %load, %load5
  %call = call i32 @left.answer()
  %add6 = add i32 %add, %call
  %call7 = call i32 @right.answer()
  %add8 = add i32 %add6, %call7
  ret i32 %add8
}
