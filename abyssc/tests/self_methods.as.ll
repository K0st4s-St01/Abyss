; ModuleID = '/home/xator/Abyss/abyssc/tests/self_methods.as'
source_filename = "/home/xator/Abyss/abyssc/tests/self_methods.as"

%Counter = type { i32 }

define i32 @Counter.bump(ptr %self, i32 %amount) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %amount2 = alloca i32, align 4
  store i32 %amount, ptr %amount2, align 4
  %load = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %load, i32 0, i32 0
  %load3 = load ptr, ptr %self1, align 8
  %value4 = getelementptr %Counter, ptr %load3, i32 0, i32 0
  %cur = load i32, ptr %value4, align 4
  %load5 = load i32, ptr %amount2, align 4
  %add = add i32 %cur, %load5
  store i32 %add, ptr %value4, align 4
  %load6 = load ptr, ptr %self1, align 8
  %value7 = getelementptr %Counter, ptr %load6, i32 0, i32 0
  %load8 = load i32, ptr %value7, align 4
  ret i32 %load8
}

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %load = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %load, i32 0, i32 0
  %load2 = load i32, ptr %value, align 4
  ret i32 %load2
}

define i32 @Counter.bump_twice(ptr %self, i32 %amount) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %amount2 = alloca i32, align 4
  store i32 %amount, ptr %amount2, align 4
  %load = load ptr, ptr %self1, align 8
  %arg = load i32, ptr %amount2, align 4
  %call = call i32 @Counter.bump(ptr %load, i32 %arg)
  %load3 = load ptr, ptr %self1, align 8
  %arg4 = load i32, ptr %amount2, align 4
  %call5 = call i32 @Counter.bump(ptr %load3, i32 %arg4)
  ret i32 %call5
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %ptr = alloca ptr, align 8
  %counter = alloca %Counter, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store ptr %counter, ptr %ptr, align 8
  %value = getelementptr %Counter, ptr %counter, i32 0, i32 0
  %value3 = getelementptr %Counter, ptr %counter, i32 0, i32 0
  store i32 10, ptr %value3, align 4
  %call = call i32 @Counter.bump(ptr %counter, i32 3)
  %load = load ptr, ptr %ptr, align 8
  %call4 = call i32 @Counter.bump_twice(ptr %load, i32 2)
  %add = add i32 %call, %call4
  %call5 = call i32 @Counter.read(ptr %counter)
  %add6 = add i32 %add, %call5
  ret i32 %add6
}
