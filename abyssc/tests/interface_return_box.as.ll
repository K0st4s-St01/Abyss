; ModuleID = '/home/xator/Abyss/abyssc/tests/interface_return_box.as'
source_filename = "/home/xator/Abyss/abyssc/tests/interface_return_box.as"

%Counter = type { i32 }
%Readable = type { ptr, ptr }

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  %add = add i32 %load, 1
  ret i32 %add
}

define ptr @as_readable(ptr %counter) {
entry:
  %counter1 = alloca ptr, align 8
  store ptr %counter, ptr %counter1, align 8
  %load = load ptr, ptr %counter1, align 8
  %load2 = load ptr, ptr %counter1, align 8
  %iface.heap = tail call ptr @malloc(i32 ptrtoint (ptr getelementptr (%Readable, ptr null, i32 1) to i32))
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.heap, i32 0, i32 0
  store ptr %load2, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.heap, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method, align 8
  ret ptr %iface.heap
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %readable = alloca ptr, align 8
  %counter = alloca %Counter, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Counter zeroinitializer, ptr %counter, align 4
  %struct.init = getelementptr %Counter, ptr %counter, i32 0, i32 0
  store i32 41, ptr %struct.init, align 4
  %call = call ptr @as_readable(ptr %counter)
  store ptr %call, ptr %readable, align 8
  %load = load ptr, ptr %readable, align 8
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj, align 8
  %iface.fn = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 %iface.fn.load(ptr %iface.obj.load)
  ret i32 %iface.call
}

declare noalias ptr @malloc(i32)
