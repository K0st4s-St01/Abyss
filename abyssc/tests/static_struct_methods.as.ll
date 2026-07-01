; ModuleID = '/home/xator/Abyss/abyssc/tests/static_struct_methods.as'
source_filename = "/home/xator/Abyss/abyssc/tests/static_struct_methods.as"

%Box = type { i32 }

define i32 @Box.base() {
entry:
  ret i32 20
}

define i32 @Box.get(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Box, ptr %self_field_base, i32 0, i32 0
  %call = call i32 @Box.base()
  %load = load i32, ptr %value, align 4
  %add = add i32 %load, %call
  %call2 = call i32 @Box.base()
  %add3 = add i32 %add, %call2
  ret i32 %add3
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %box = alloca %Box, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Box zeroinitializer, ptr %box, align 4
  %struct.init = getelementptr %Box, ptr %box, i32 0, i32 0
  store i32 2, ptr %struct.init, align 4
  %call = call i32 @Box.base()
  %call3 = call i32 @Box.get(ptr %box)
  %add = add i32 %call, %call3
  ret i32 %add
}
