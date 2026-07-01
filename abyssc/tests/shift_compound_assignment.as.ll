; ModuleID = '/home/xator/Abyss/abyssc/tests/shift_compound_assignment.as'
source_filename = "/home/xator/Abyss/abyssc/tests/shift_compound_assignment.as"

%Box = type { i32 }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %ptr = alloca ptr, align 8
  %box = alloca %Box, align 8
  %nums = alloca [2 x i32], align 4
  %value = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 1, ptr %value, align 4
  %cur = load i32, ptr %value, align 4
  %shl = shl i32 %cur, 3
  store i32 %shl, ptr %value, align 4
  %cur3 = load i32, ptr %value, align 4
  %ashr = ashr i32 %cur3, 1
  store i32 %ashr, ptr %value, align 4
  store [2 x i32] zeroinitializer, ptr %nums, align 4
  %arr.init = getelementptr [2 x i32], ptr %nums, i32 0, i32 0
  store i32 2, ptr %arr.init, align 4
  %arr.init4 = getelementptr [2 x i32], ptr %nums, i32 0, i32 1
  store i32 4, ptr %arr.init4, align 4
  %idx = getelementptr [2 x i32], ptr %nums, i32 0, i32 0
  %idx5 = getelementptr [2 x i32], ptr %nums, i32 0, i32 0
  %cur6 = load i32, ptr %idx5, align 4
  %shl7 = shl i32 %cur6, 2
  store i32 %shl7, ptr %idx5, align 4
  store %Box zeroinitializer, ptr %box, align 4
  %struct.init = getelementptr %Box, ptr %box, i32 0, i32 0
  store i32 3, ptr %struct.init, align 4
  %value8 = getelementptr %Box, ptr %box, i32 0, i32 0
  %value9 = getelementptr %Box, ptr %box, i32 0, i32 0
  %cur10 = load i32, ptr %value9, align 4
  %shl11 = shl i32 %cur10, 2
  store i32 %shl11, ptr %value9, align 4
  store ptr %value, ptr %ptr, align 8
  %load = load ptr, ptr %ptr, align 8
  %load12 = load ptr, ptr %ptr, align 8
  %cur13 = load i32, ptr %load12, align 4
  %shl14 = shl i32 %cur13, 1
  store i32 %shl14, ptr %load12, align 4
  %idx15 = getelementptr [2 x i32], ptr %nums, i32 0, i32 0
  %load16 = load i32, ptr %value, align 4
  %load17 = load i32, ptr %idx15, align 4
  %add = add i32 %load16, %load17
  %value18 = getelementptr %Box, ptr %box, i32 0, i32 0
  %load19 = load i32, ptr %value18, align 4
  %add20 = add i32 %add, %load19
  ret i32 %add20
}
