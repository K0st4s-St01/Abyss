; ModuleID = 'main.as'
source_filename = "main.as"

%Point = type { i32, i32 }

define i32 @Point.length_sq(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %load = load ptr, ptr %self1, align 8
  %x = getelementptr %Point, ptr %load, i32 0, i32 0
  %load2 = load ptr, ptr %self1, align 8
  %x3 = getelementptr %Point, ptr %load2, i32 0, i32 0
  %load4 = load i32, ptr %x, align 4
  %load5 = load i32, ptr %x3, align 4
  %mul = mul i32 %load4, %load5
  %load6 = load ptr, ptr %self1, align 8
  %y = getelementptr %Point, ptr %load6, i32 0, i32 1
  %load7 = load ptr, ptr %self1, align 8
  %y8 = getelementptr %Point, ptr %load7, i32 0, i32 1
  %load9 = load i32, ptr %y, align 4
  %load10 = load i32, ptr %y8, align 4
  %mul11 = mul i32 %load9, %load10
  %add = add i32 %mul, %mul11
  ret i32 %add
}

define i32 @Point.dot(ptr %self, %Point %other) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %other2 = alloca %Point, align 8
  store %Point %other, ptr %other2, align 4
  %load = load ptr, ptr %self1, align 8
  %x = getelementptr %Point, ptr %load, i32 0, i32 0
  %x3 = getelementptr %Point, ptr %other2, i32 0, i32 0
  %load4 = load i32, ptr %x, align 4
  %load5 = load i32, ptr %x3, align 4
  %mul = mul i32 %load4, %load5
  %load6 = load ptr, ptr %self1, align 8
  %y = getelementptr %Point, ptr %load6, i32 0, i32 1
  %y7 = getelementptr %Point, ptr %other2, i32 0, i32 1
  %load8 = load i32, ptr %y, align 4
  %load9 = load i32, ptr %y7, align 4
  %mul10 = mul i32 %load8, %load9
  %add = add i32 %mul, %mul10
  ret i32 %add
}

define %Point @Point.make(i32 %x, i32 %y) {
entry:
  %p = alloca %Point, align 8
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %y2 = alloca i32, align 4
  store i32 %y, ptr %y2, align 4
  store %Point zeroinitializer, ptr %p, align 4
  %struct.init = getelementptr %Point, ptr %p, i32 0, i32 0
  %load = load i32, ptr %x1, align 4
  store i32 %load, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %p, i32 0, i32 1
  %load4 = load i32, ptr %y2, align 4
  store i32 %load4, ptr %struct.init3, align 4
  %load5 = load %Point, ptr %p, align 4
  ret %Point %load5
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %b = alloca %Point, align 8
  %a = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call %Point @Point.make(i32 3, i32 4)
  store %Point %call, ptr %a, align 4
  %call3 = call %Point @Point.make(i32 1, i32 2)
  store %Point %call3, ptr %b, align 4
  %call4 = call i32 @Point.length_sq(ptr %a)
  %arg = load %Point, ptr %b, align 4
  %call5 = call i32 @Point.dot(ptr %a, %Point %arg)
  %add = add i32 %call4, %call5
  ret i32 %add
}
