; ModuleID = '/home/xator/Abyss/abyssc/tests/nested_aggregates.as'
source_filename = "/home/xator/Abyss/abyssc/tests/nested_aggregates.as"

%Point = type { i32, i32 }
%Grid = type { [2 x %Point], [3 x i32] }

@global_points = global [2 x %Point] [%Point { i32 1, i32 2 }, %Point { i32 3, i32 4 }]
@global_grid = global %Grid { [2 x %Point] [%Point { i32 5, i32 6 }, %Point { i32 7, i32 8 }], [3 x i32] [i32 9, i32 10, i32 0] }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %local_grid = alloca %Grid, align 8
  %local_points = alloca [2 x %Point], align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store [2 x %Point] zeroinitializer, ptr %local_points, align 4
  %arr.init = getelementptr [2 x %Point], ptr %local_points, i32 0, i32 0
  store %Point zeroinitializer, ptr %arr.init, align 4
  %struct.init = getelementptr %Point, ptr %arr.init, i32 0, i32 0
  store i32 11, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %arr.init, i32 0, i32 1
  store i32 12, ptr %struct.init3, align 4
  %arr.init4 = getelementptr [2 x %Point], ptr %local_points, i32 0, i32 1
  store %Point zeroinitializer, ptr %arr.init4, align 4
  %struct.init5 = getelementptr %Point, ptr %arr.init4, i32 0, i32 0
  store i32 13, ptr %struct.init5, align 4
  %struct.init6 = getelementptr %Point, ptr %arr.init4, i32 0, i32 1
  store i32 14, ptr %struct.init6, align 4
  store %Grid zeroinitializer, ptr %local_grid, align 4
  %struct.init7 = getelementptr %Grid, ptr %local_grid, i32 0, i32 0
  store [2 x %Point] zeroinitializer, ptr %struct.init7, align 4
  %arr.init8 = getelementptr [2 x %Point], ptr %struct.init7, i32 0, i32 0
  store %Point zeroinitializer, ptr %arr.init8, align 4
  %struct.init9 = getelementptr %Point, ptr %arr.init8, i32 0, i32 0
  store i32 15, ptr %struct.init9, align 4
  %struct.init10 = getelementptr %Point, ptr %arr.init8, i32 0, i32 1
  store i32 16, ptr %struct.init10, align 4
  %arr.init11 = getelementptr [2 x %Point], ptr %struct.init7, i32 0, i32 1
  store %Point zeroinitializer, ptr %arr.init11, align 4
  %struct.init12 = getelementptr %Point, ptr %arr.init11, i32 0, i32 0
  store i32 17, ptr %struct.init12, align 4
  %struct.init13 = getelementptr %Point, ptr %arr.init11, i32 0, i32 1
  store i32 18, ptr %struct.init13, align 4
  %struct.init14 = getelementptr %Grid, ptr %local_grid, i32 0, i32 1
  store [3 x i32] zeroinitializer, ptr %struct.init14, align 4
  %arr.init15 = getelementptr [3 x i32], ptr %struct.init14, i32 0, i32 0
  store i32 19, ptr %arr.init15, align 4
  %arr.init16 = getelementptr [3 x i32], ptr %struct.init14, i32 0, i32 1
  store i32 20, ptr %arr.init16, align 4
  %arr.init17 = getelementptr [3 x i32], ptr %struct.init14, i32 0, i32 2
  store i32 21, ptr %arr.init17, align 4
  %load = load i32, ptr @global_points, align 4
  %load18 = load i32, ptr getelementptr (%Point, ptr getelementptr ([2 x %Point], ptr @global_points, i32 0, i32 1), i32 0, i32 1), align 4
  %add = add i32 %load, %load18
  %load19 = load i32, ptr @global_grid, align 4
  %add20 = add i32 %add, %load19
  %load21 = load i32, ptr getelementptr (%Point, ptr getelementptr ([2 x %Point], ptr @global_grid, i32 0, i32 1), i32 0, i32 1), align 4
  %add22 = add i32 %add20, %load21
  %load23 = load i32, ptr getelementptr (%Grid, ptr @global_grid, i32 0, i32 1), align 4
  %add24 = add i32 %add22, %load23
  %load25 = load i32, ptr getelementptr ([3 x i32], ptr getelementptr (%Grid, ptr @global_grid, i32 0, i32 1), i32 0, i32 2), align 4
  %add26 = add i32 %add24, %load25
  %idx = getelementptr [2 x %Point], ptr %local_points, i32 0, i32 0
  %x = getelementptr %Point, ptr %idx, i32 0, i32 0
  %load27 = load i32, ptr %x, align 4
  %add28 = add i32 %add26, %load27
  %idx29 = getelementptr [2 x %Point], ptr %local_points, i32 0, i32 1
  %y = getelementptr %Point, ptr %idx29, i32 0, i32 1
  %load30 = load i32, ptr %y, align 4
  %add31 = add i32 %add28, %load30
  %points = getelementptr %Grid, ptr %local_grid, i32 0, i32 0
  %idx32 = getelementptr [2 x %Point], ptr %points, i32 0, i32 0
  %x33 = getelementptr %Point, ptr %idx32, i32 0, i32 0
  %load34 = load i32, ptr %x33, align 4
  %add35 = add i32 %add31, %load34
  %points36 = getelementptr %Grid, ptr %local_grid, i32 0, i32 0
  %idx37 = getelementptr [2 x %Point], ptr %points36, i32 0, i32 1
  %y38 = getelementptr %Point, ptr %idx37, i32 0, i32 1
  %load39 = load i32, ptr %y38, align 4
  %add40 = add i32 %add35, %load39
  %weights = getelementptr %Grid, ptr %local_grid, i32 0, i32 1
  %idx41 = getelementptr [3 x i32], ptr %weights, i32 0, i32 0
  %load42 = load i32, ptr %idx41, align 4
  %add43 = add i32 %add40, %load42
  %weights44 = getelementptr %Grid, ptr %local_grid, i32 0, i32 1
  %idx45 = getelementptr [3 x i32], ptr %weights44, i32 0, i32 2
  %load46 = load i32, ptr %idx45, align 4
  %add47 = add i32 %add43, %load46
  ret i32 %add47
}
