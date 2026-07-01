; ModuleID = 'main.as'
source_filename = "main.as"

%Pair = type { i32, i32 }

@arithmetic = global i32 14
@shifted = global i32 16
@masked = global i32 12
@inverted = global i32 0
@pair = global %Pair { i32 14, i32 12 }
@values = global [3 x i32] [i32 14, i32 16, i32 12]

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %load = load i32, ptr @pair, align 4
  %load3 = load i32, ptr getelementptr (%Pair, ptr @pair, i32 0, i32 1), align 4
  %add = add i32 %load, %load3
  %load4 = load i32, ptr getelementptr ([3 x i32], ptr @values, i32 0, i32 1), align 4
  %add5 = add i32 %add, %load4
  %load6 = load i32, ptr @inverted, align 4
  %add7 = add i32 %add5, %load6
  ret i32 %add7
}
