; ModuleID = '/home/xator/Abyss/abyssc/tests/struct_initializers.as'
source_filename = "/home/xator/Abyss/abyssc/tests/struct_initializers.as"

%Point = type { i32, i32 }
%Pair = type { i32, i32, i32 }

@origin = global %Point { i32 1, i32 2 }
@global_partial = global %Pair { i32 6, i32 0, i32 0 }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %partial = alloca %Pair, align 8
  %full = alloca %Pair, align 8
  %p = alloca %Point, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Point zeroinitializer, ptr %p, align 4
  %struct.init = getelementptr %Point, ptr %p, i32 0, i32 0
  store i32 7, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %p, i32 0, i32 1
  store i32 9, ptr %struct.init3, align 4
  store %Pair zeroinitializer, ptr %full, align 4
  %struct.init4 = getelementptr %Pair, ptr %full, i32 0, i32 0
  store i32 3, ptr %struct.init4, align 4
  %struct.init5 = getelementptr %Pair, ptr %full, i32 0, i32 1
  store i32 4, ptr %struct.init5, align 4
  %struct.init6 = getelementptr %Pair, ptr %full, i32 0, i32 2
  store i32 5, ptr %struct.init6, align 4
  store %Pair zeroinitializer, ptr %partial, align 4
  %struct.init7 = getelementptr %Pair, ptr %partial, i32 0, i32 0
  store i32 8, ptr %struct.init7, align 4
  %x = getelementptr %Point, ptr %p, i32 0, i32 0
  %y = getelementptr %Point, ptr %p, i32 0, i32 1
  %load = load i32, ptr %x, align 4
  %load8 = load i32, ptr %y, align 4
  %add = add i32 %load, %load8
  %first = getelementptr %Pair, ptr %full, i32 0, i32 0
  %load9 = load i32, ptr %first, align 4
  %add10 = add i32 %add, %load9
  %second = getelementptr %Pair, ptr %full, i32 0, i32 1
  %load11 = load i32, ptr %second, align 4
  %add12 = add i32 %add10, %load11
  %third = getelementptr %Pair, ptr %full, i32 0, i32 2
  %load13 = load i32, ptr %third, align 4
  %add14 = add i32 %add12, %load13
  %first15 = getelementptr %Pair, ptr %partial, i32 0, i32 0
  %load16 = load i32, ptr %first15, align 4
  %add17 = add i32 %add14, %load16
  %second18 = getelementptr %Pair, ptr %partial, i32 0, i32 1
  %load19 = load i32, ptr %second18, align 4
  %add20 = add i32 %add17, %load19
  %third21 = getelementptr %Pair, ptr %partial, i32 0, i32 2
  %load22 = load i32, ptr %third21, align 4
  %add23 = add i32 %add20, %load22
  %load24 = load i32, ptr @origin, align 4
  %add25 = add i32 %add23, %load24
  %load26 = load i32, ptr getelementptr (%Point, ptr @origin, i32 0, i32 1), align 4
  %add27 = add i32 %add25, %load26
  %load28 = load i32, ptr @global_partial, align 4
  %add29 = add i32 %add27, %load28
  %load30 = load i32, ptr getelementptr (%Pair, ptr @global_partial, i32 0, i32 1), align 4
  %add31 = add i32 %add29, %load30
  %load32 = load i32, ptr getelementptr (%Pair, ptr @global_partial, i32 0, i32 2), align 4
  %add33 = add i32 %add31, %load32
  ret i32 %add33
}
