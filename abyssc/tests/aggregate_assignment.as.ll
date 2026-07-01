; ModuleID = '/home/xator/Abyss/abyssc/tests/aggregate_assignment.as'
source_filename = "/home/xator/Abyss/abyssc/tests/aggregate_assignment.as"

%Holder = type { [2 x %Point], [3 x i32] }
%Point = type { i32, i32 }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %holder = alloca %Holder, align 8
  %point = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Point zeroinitializer, ptr %point, align 4
  %struct.init = getelementptr %Point, ptr %point, i32 0, i32 0
  store i32 1, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %point, i32 0, i32 1
  store i32 2, ptr %struct.init3, align 4
  store %Point zeroinitializer, ptr %point, align 4
  %struct.init4 = getelementptr %Point, ptr %point, i32 0, i32 0
  store i32 3, ptr %struct.init4, align 4
  %struct.init5 = getelementptr %Point, ptr %point, i32 0, i32 1
  store i32 4, ptr %struct.init5, align 4
  %assign.load = load %Point, ptr %point, align 4
  %points = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx = getelementptr [2 x %Point], ptr %points, i32 0, i32 0
  store %Point zeroinitializer, ptr %idx, align 4
  %struct.init6 = getelementptr %Point, ptr %idx, i32 0, i32 0
  store i32 5, ptr %struct.init6, align 4
  %struct.init7 = getelementptr %Point, ptr %idx, i32 0, i32 1
  store i32 6, ptr %struct.init7, align 4
  %assign.load8 = load %Point, ptr %idx, align 4
  %points9 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx10 = getelementptr [2 x %Point], ptr %points9, i32 0, i32 1
  store %Point zeroinitializer, ptr %idx10, align 4
  %struct.init11 = getelementptr %Point, ptr %idx10, i32 0, i32 0
  store i32 7, ptr %struct.init11, align 4
  %struct.init12 = getelementptr %Point, ptr %idx10, i32 0, i32 1
  store i32 8, ptr %struct.init12, align 4
  %assign.load13 = load %Point, ptr %idx10, align 4
  %weights = getelementptr %Holder, ptr %holder, i32 0, i32 1
  store [3 x i32] zeroinitializer, ptr %weights, align 4
  %arr.init = getelementptr [3 x i32], ptr %weights, i32 0, i32 0
  store i32 9, ptr %arr.init, align 4
  %arr.init14 = getelementptr [3 x i32], ptr %weights, i32 0, i32 1
  store i32 10, ptr %arr.init14, align 4
  %arr.init15 = getelementptr [3 x i32], ptr %weights, i32 0, i32 2
  store i32 11, ptr %arr.init15, align 4
  %assign.load16 = load [3 x i32], ptr %weights, align 4
  %x = getelementptr %Point, ptr %point, i32 0, i32 0
  %y = getelementptr %Point, ptr %point, i32 0, i32 1
  %load = load i32, ptr %x, align 4
  %load17 = load i32, ptr %y, align 4
  %add = add i32 %load, %load17
  %points18 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx19 = getelementptr [2 x %Point], ptr %points18, i32 0, i32 0
  %x20 = getelementptr %Point, ptr %idx19, i32 0, i32 0
  %load21 = load i32, ptr %x20, align 4
  %add22 = add i32 %add, %load21
  %points23 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx24 = getelementptr [2 x %Point], ptr %points23, i32 0, i32 0
  %y25 = getelementptr %Point, ptr %idx24, i32 0, i32 1
  %load26 = load i32, ptr %y25, align 4
  %add27 = add i32 %add22, %load26
  %points28 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx29 = getelementptr [2 x %Point], ptr %points28, i32 0, i32 1
  %x30 = getelementptr %Point, ptr %idx29, i32 0, i32 0
  %load31 = load i32, ptr %x30, align 4
  %add32 = add i32 %add27, %load31
  %points33 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %idx34 = getelementptr [2 x %Point], ptr %points33, i32 0, i32 1
  %y35 = getelementptr %Point, ptr %idx34, i32 0, i32 1
  %load36 = load i32, ptr %y35, align 4
  %add37 = add i32 %add32, %load36
  %weights38 = getelementptr %Holder, ptr %holder, i32 0, i32 1
  %idx39 = getelementptr [3 x i32], ptr %weights38, i32 0, i32 0
  %load40 = load i32, ptr %idx39, align 4
  %add41 = add i32 %add37, %load40
  %weights42 = getelementptr %Holder, ptr %holder, i32 0, i32 1
  %idx43 = getelementptr [3 x i32], ptr %weights42, i32 0, i32 1
  %load44 = load i32, ptr %idx43, align 4
  %add45 = add i32 %add41, %load44
  %weights46 = getelementptr %Holder, ptr %holder, i32 0, i32 1
  %idx47 = getelementptr [3 x i32], ptr %weights46, i32 0, i32 2
  %load48 = load i32, ptr %idx47, align 4
  %add49 = add i32 %add45, %load48
  ret i32 %add49
}
