; ModuleID = '/home/xator/Abyss/abyssc/tests/array_decay.as'
source_filename = "/home/xator/Abyss/abyssc/tests/array_decay.as"

%Bag = type { [2 x i32] }

@global_values = global [3 x i32] [i32 2, i32 4, i32 6]

define i32 @sum3(ptr %values) {
entry:
  %values1 = alloca ptr, align 8
  store ptr %values, ptr %values1, align 8
  %deref = load ptr, ptr %values1, align 8
  %idx = getelementptr i32, ptr %deref, i32 0
  %deref2 = load ptr, ptr %values1, align 8
  %idx3 = getelementptr i32, ptr %deref2, i32 1
  %load = load i32, ptr %idx, align 4
  %load4 = load i32, ptr %idx3, align 4
  %add = add i32 %load, %load4
  %deref5 = load ptr, ptr %values1, align 8
  %idx6 = getelementptr i32, ptr %deref5, i32 2
  %load7 = load i32, ptr %idx6, align 4
  %add8 = add i32 %add, %load7
  ret i32 %add8
}

define i32 @sum2(ptr %values) {
entry:
  %values1 = alloca ptr, align 8
  store ptr %values, ptr %values1, align 8
  %deref = load ptr, ptr %values1, align 8
  %idx = getelementptr i32, ptr %deref, i32 0
  %deref2 = load ptr, ptr %values1, align 8
  %idx3 = getelementptr i32, ptr %deref2, i32 1
  %load = load i32, ptr %idx, align 4
  %load4 = load i32, ptr %idx3, align 4
  %add = add i32 %load, %load4
  ret i32 %add
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %bag = alloca %Bag, align 8
  %local_values = alloca [3 x i32], align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store [3 x i32] zeroinitializer, ptr %local_values, align 4
  %arr.init = getelementptr [3 x i32], ptr %local_values, i32 0, i32 0
  store i32 3, ptr %arr.init, align 4
  %arr.init3 = getelementptr [3 x i32], ptr %local_values, i32 0, i32 1
  store i32 5, ptr %arr.init3, align 4
  %arr.init4 = getelementptr [3 x i32], ptr %local_values, i32 0, i32 2
  store i32 7, ptr %arr.init4, align 4
  %values = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx = getelementptr [2 x i32], ptr %values, i32 0, i32 0
  %values5 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx6 = getelementptr [2 x i32], ptr %values5, i32 0, i32 0
  store i32 11, ptr %idx6, align 4
  %values7 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx8 = getelementptr [2 x i32], ptr %values7, i32 0, i32 1
  %values9 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx10 = getelementptr [2 x i32], ptr %values9, i32 0, i32 1
  store i32 13, ptr %idx10, align 4
  %array.decay = getelementptr [3 x i32], ptr %local_values, i32 0, i32 0
  %call = call i32 @sum3(ptr %array.decay)
  %call11 = call i32 @sum3(ptr @global_values)
  %add = add i32 %call, %call11
  %values12 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %array.decay13 = getelementptr [2 x i32], ptr %values12, i32 0, i32 0
  %call14 = call i32 @sum2(ptr %array.decay13)
  %add15 = add i32 %add, %call14
  ret i32 %add15
}
