; ModuleID = '/home/xator/Abyss/abyssc/tests/import_static_type_method_link.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_static_type_method_link.as"

declare i32 @left.Counter.read(ptr)

declare i32 @left.Counter.base()

declare i32 @Counter.read(ptr)

declare i32 @Counter.base()

declare i32 @left.answer()

declare i32 @answer()

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @Counter.base()
  %call3 = call i32 @answer()
  %add = add i32 %call, %call3
  ret i32 %add
}
