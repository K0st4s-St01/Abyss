; ModuleID = '/home/xator/Abyss/abyssc/tests/aggregate_arguments.as'
source_filename = "/home/xator/Abyss/abyssc/tests/aggregate_arguments.as"

%Point = type { i32, i32 }

define %Point @make_point(i32 %x, i32 %y) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, ptr %y2, align 4
  %agg.tmp = alloca %Point, align 8
  store %Point zeroinitializer, ptr %agg.tmp, align 4
  %struct.init = getelementptr %Point, ptr %agg.tmp, i32 0, i32 0
  %load = load i32, ptr %x1, align 4
  store i32 %load, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %agg.tmp, i32 0, i32 1
  %load4 = load i32, ptr %y2, align 4
  store i32 %load4, ptr %struct.init3, align 4
  %agg.load = load %Point, ptr %agg.tmp, align 4
  ret %Point %agg.load
}

define i32 @sum_point(%Point %point) {
entry:
  %point1 = alloca %Point, align 8
  store %Point %point, ptr %point1, align 4
  %x = getelementptr %Point, ptr %point1, i32 0, i32 0
  %y = getelementptr %Point, ptr %point1, i32 0, i32 1
  %load = load i32, ptr %x, align 4
  %load2 = load i32, ptr %y, align 4
  %add = add i32 %load, %load2
  ret i32 %add
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %made = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call %Point @make_point(i32 7, i32 8)
  store %Point %call, ptr %made, align 4
  %agg.tmp = alloca %Point, align 8
  store %Point zeroinitializer, ptr %agg.tmp, align 4
  %struct.init = getelementptr %Point, ptr %agg.tmp, i32 0, i32 0
  store i32 5, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %agg.tmp, i32 0, i32 1
  store i32 6, ptr %struct.init3, align 4
  %agg.load = load %Point, ptr %agg.tmp, align 4
  %call4 = call i32 @sum_point(%Point %agg.load)
  %arg = load %Point, ptr %made, align 4
  %call5 = call i32 @sum_point(%Point %arg)
  %add = add i32 %call4, %call5
  ret i32 %add
}
