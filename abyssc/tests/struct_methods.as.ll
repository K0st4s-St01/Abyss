; ModuleID = '/home/xator/Abyss/abyssc/tests/struct_methods.as'
source_filename = "/home/xator/Abyss/abyssc/tests/struct_methods.as"

%Box = type { i32 }

define i32 @Box.get(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Box, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}

define i32 @Box.set(ptr %self, i32 %next) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %next2 = alloca i32, align 4
  store i32 %next, ptr %next2, align 4
  %load = load i32, ptr %next2, align 4
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Box, ptr %self_field_base, i32 0, i32 0
  store i32 %load, ptr %value, align 4
  %self_field_base3 = load ptr, ptr %self1, align 8
  %value4 = getelementptr %Box, ptr %self_field_base3, i32 0, i32 0
  %self_field_base5 = load ptr, ptr %self1, align 8
  %value6 = getelementptr %Box, ptr %self_field_base5, i32 0, i32 0
  %cur = load i32, ptr %value6, align 4
  %add = add i32 %cur, 0
  store i32 %add, ptr %value6, align 4
  %self_field_base7 = load ptr, ptr %self1, align 8
  %value8 = getelementptr %Box, ptr %self_field_base7, i32 0, i32 0
  %load9 = load i32, ptr %value8, align 4
  ret i32 %load9
}

define i32 @Box.add(ptr %self, i32 %amount) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %amount2 = alloca i32, align 4
  store i32 %amount, ptr %amount2, align 4
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Box, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  %load3 = load i32, ptr %amount2, align 4
  %add = add i32 %load, %load3
  ret i32 %add
}

define i32 @Box.twice(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_arg = load ptr, ptr %self1, align 8
  %call = call i32 @Box.get(ptr %self_arg)
  %self_arg2 = load ptr, ptr %self1, align 8
  %call3 = call i32 @Box.add(ptr %self_arg2, i32 0)
  %add = add i32 %call, %call3
  ret i32 %add
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %ptr = alloca ptr, align 8
  %box = alloca %Box, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store ptr %box, ptr %ptr, align 8
  %call = call i32 @Box.set(ptr %box, i32 5)
  %load = load ptr, ptr %ptr, align 8
  %call3 = call i32 @Box.get(ptr %load)
  %add = add i32 %call, %call3
  %call4 = call i32 @Box.add(ptr %box, i32 2)
  %add5 = add i32 %add, %call4
  %load6 = load ptr, ptr %ptr, align 8
  %call7 = call i32 @Box.twice(ptr %load6)
  %add8 = add i32 %add5, %call7
  ret i32 %add8
}
