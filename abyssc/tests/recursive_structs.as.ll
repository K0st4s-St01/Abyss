; ModuleID = '/home/xator/Abyss/abyssc/tests/recursive_structs.as'
source_filename = "/home/xator/Abyss/abyssc/tests/recursive_structs.as"

%Node = type { i32, ptr }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %second = alloca %Node, align 8
  %first = alloca %Node, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Node zeroinitializer, ptr %first, align 8
  %struct.init = getelementptr %Node, ptr %first, i32 0, i32 0
  store i32 3, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Node, ptr %first, i32 0, i32 1
  store ptr null, ptr %struct.init3, align 8
  store %Node zeroinitializer, ptr %second, align 8
  %struct.init4 = getelementptr %Node, ptr %second, i32 0, i32 0
  store i32 4, ptr %struct.init4, align 4
  %struct.init5 = getelementptr %Node, ptr %second, i32 0, i32 1
  store ptr %first, ptr %struct.init5, align 8
  %value = getelementptr %Node, ptr %second, i32 0, i32 0
  %next = getelementptr %Node, ptr %second, i32 0, i32 1
  %load = load ptr, ptr %next, align 8
  %value6 = getelementptr %Node, ptr %load, i32 0, i32 0
  %load7 = load i32, ptr %value, align 4
  %load8 = load i32, ptr %value6, align 4
  %add = add i32 %load7, %load8
  ret i32 %add
}
