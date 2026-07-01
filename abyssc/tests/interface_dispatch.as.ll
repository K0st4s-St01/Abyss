; ModuleID = '/home/xator/Abyss/abyssc/tests/interface_dispatch.as'
source_filename = "/home/xator/Abyss/abyssc/tests/interface_dispatch.as"

%Counter = type { i32 }
%Readable = type { ptr, ptr }

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  %add = add i32 %load, 2
  ret i32 %add
}

define i32 @take(ptr %item) {
entry:
  %item1 = alloca ptr, align 8
  store ptr %item, ptr %item1, align 8
  %load = load ptr, ptr %item1, align 8
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj, align 8
  %iface.fn = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 %iface.fn.load(ptr %iface.obj.load)
  ret i32 %iface.call
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp4 = alloca %Readable, align 8
  %iface.tmp = alloca %Readable, align 8
  %readable = alloca ptr, align 8
  %counter = alloca %Counter, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Counter zeroinitializer, ptr %counter, align 4
  %struct.init = getelementptr %Counter, ptr %counter, i32 0, i32 0
  store i32 40, ptr %struct.init, align 4
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 0
  store ptr %counter, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method, align 8
  store ptr %iface.tmp, ptr %readable, align 8
  %load = load ptr, ptr %readable, align 8
  %iface.obj3 = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj3, align 8
  %iface.fn = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 %iface.fn.load(ptr %iface.obj.load)
  %iface.obj5 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 0
  store ptr %counter, ptr %iface.obj5, align 8
  %iface.method6 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method6, align 8
  %call = call i32 @take(ptr %iface.tmp4)
  %add = add i32 %iface.call, %call
  %sub = sub i32 %add, 42
  ret i32 %sub
}
