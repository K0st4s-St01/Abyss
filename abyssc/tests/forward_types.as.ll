; ModuleID = '/home/xator/Abyss/abyssc/tests/forward_types.as'
source_filename = "/home/xator/Abyss/abyssc/tests/forward_types.as"

%Point = type { i32, i32 }
%Readable = type { ptr, ptr }
%Counter = type { i32 }

define i32 @use_point(%Point %p) {
entry:
  %p1 = alloca %Point, align 8
  store %Point %p, ptr %p1, align 4
  %x = getelementptr %Point, ptr %p1, i32 0, i32 0
  %load = load i32, ptr %x, align 4
  %add = add i32 %load, 3
  ret i32 %add
}

define i32 @accepts(ptr %item) {
entry:
  %item1 = alloca ptr, align 8
  store ptr %item, ptr %item1, align 8
  %load = load ptr, ptr %item1, align 8
  %cond = icmp ne ptr %load, null
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 5

if.end:                                           ; preds = %entry
  ret i32 1
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp = alloca %Readable, align 8
  %counter = alloca %Counter, align 8
  %p = alloca %Point, align 8
  %base = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 2, ptr %base, align 4
  store %Point zeroinitializer, ptr %p, align 4
  %struct.init = getelementptr %Point, ptr %p, i32 0, i32 0
  %load = load i32, ptr %base, align 4
  store i32 %load, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %p, i32 0, i32 1
  store i32 4, ptr %struct.init3, align 4
  store %Counter zeroinitializer, ptr %counter, align 4
  %struct.init4 = getelementptr %Counter, ptr %counter, i32 0, i32 0
  store i32 6, ptr %struct.init4, align 4
  %arg = load %Point, ptr %p, align 4
  %call = call i32 @use_point(%Point %arg)
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 0
  store ptr %counter, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method, align 8
  %call5 = call i32 @accepts(ptr %iface.tmp)
  %add = add i32 %call, %call5
  ret i32 %add
}

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}
