; ModuleID = '/home/xator/Abyss/abyssc/tests/fixed_arrays.as'
source_filename = "/home/xator/Abyss/abyssc/tests/fixed_arrays.as"

%Bag = type { [3 x i32] }

@totals = global [2 x i32] [i32 12, i32 4]
@partial = global [3 x i32] [i32 8, i32 0, i32 0]

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %bag = alloca %Bag, align 8
  %nums = alloca [4 x i32], align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store [4 x i32] zeroinitializer, ptr %nums, align 4
  %arr.init = getelementptr [4 x i32], ptr %nums, i32 0, i32 0
  store i32 5, ptr %arr.init, align 4
  %arr.init3 = getelementptr [4 x i32], ptr %nums, i32 0, i32 1
  store i32 7, ptr %arr.init3, align 4
  %arr.init4 = getelementptr [4 x i32], ptr %nums, i32 0, i32 2
  store i32 0, ptr %arr.init4, align 4
  %arr.init5 = getelementptr [4 x i32], ptr %nums, i32 0, i32 3
  store i32 3, ptr %arr.init5, align 4
  %idx = getelementptr [4 x i32], ptr %nums, i32 0, i32 2
  %idx6 = getelementptr [4 x i32], ptr %nums, i32 0, i32 0
  %idx7 = getelementptr [4 x i32], ptr %nums, i32 0, i32 1
  %load = load i32, ptr %idx6, align 4
  %load8 = load i32, ptr %idx7, align 4
  %add = add i32 %load, %load8
  %idx9 = getelementptr [4 x i32], ptr %nums, i32 0, i32 2
  store i32 %add, ptr %idx9, align 4
  %values = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx10 = getelementptr [3 x i32], ptr %values, i32 0, i32 0
  %values11 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx12 = getelementptr [3 x i32], ptr %values11, i32 0, i32 0
  %load13 = load i32, ptr @totals, align 4
  store i32 %load13, ptr %idx12, align 4
  %values14 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx15 = getelementptr [3 x i32], ptr %values14, i32 0, i32 1
  %values16 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx17 = getelementptr [3 x i32], ptr %values16, i32 0, i32 1
  %load18 = load i32, ptr getelementptr ([2 x i32], ptr @totals, i32 0, i32 1), align 4
  store i32 %load18, ptr %idx17, align 4
  %values19 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx20 = getelementptr [3 x i32], ptr %values19, i32 0, i32 2
  %values21 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx22 = getelementptr [3 x i32], ptr %values21, i32 0, i32 2
  store i32 1, ptr %idx22, align 4
  %idx23 = getelementptr [4 x i32], ptr %nums, i32 0, i32 2
  %idx24 = getelementptr [4 x i32], ptr %nums, i32 0, i32 3
  %load25 = load i32, ptr %idx23, align 4
  %load26 = load i32, ptr %idx24, align 4
  %add27 = add i32 %load25, %load26
  %load28 = load i32, ptr @partial, align 4
  %add29 = add i32 %add27, %load28
  %load30 = load i32, ptr getelementptr ([3 x i32], ptr @partial, i32 0, i32 1), align 4
  %add31 = add i32 %add29, %load30
  %load32 = load i32, ptr getelementptr ([2 x i32], ptr @totals, i32 0, i32 1), align 4
  %add33 = add i32 %add31, %load32
  %values34 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx35 = getelementptr [3 x i32], ptr %values34, i32 0, i32 0
  %load36 = load i32, ptr %idx35, align 4
  %add37 = add i32 %add33, %load36
  %values38 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx39 = getelementptr [3 x i32], ptr %values38, i32 0, i32 1
  %load40 = load i32, ptr %idx39, align 4
  %add41 = add i32 %add37, %load40
  %values42 = getelementptr %Bag, ptr %bag, i32 0, i32 0
  %idx43 = getelementptr [3 x i32], ptr %values42, i32 0, i32 2
  %load44 = load i32, ptr %idx43, align 4
  %add45 = add i32 %add41, %load44
  ret i32 %add45
}
