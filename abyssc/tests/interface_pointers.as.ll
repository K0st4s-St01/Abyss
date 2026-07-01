; ModuleID = '/home/xator/Abyss/abyssc/tests/interface_pointers.as'
source_filename = "/home/xator/Abyss/abyssc/tests/interface_pointers.as"

%Counter = type { i32 }
%Readable = type { ptr, ptr }

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}

define ptr @keep(ptr %item) {
entry:
  %item1 = alloca ptr, align 8
  store ptr %item, ptr %item1, align 8
  %load = load ptr, ptr %item1, align 8
  ret ptr %load
}

define i32 @accepts(ptr %item) {
entry:
  %item1 = alloca ptr, align 8
  store ptr %item, ptr %item1, align 8
  %load = load ptr, ptr %item1, align 8
  %cond = icmp ne ptr %load, null
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  ret i32 14

if.end:                                           ; preds = %entry
  ret i32 1
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
  store i32 9, ptr %struct.init, align 4
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 0
  store ptr %counter, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method, align 8
  %call = call ptr @keep(ptr %iface.tmp)
  store ptr %call, ptr %readable, align 8
  %arg = load ptr, ptr %readable, align 8
  %call3 = call i32 @accepts(ptr %arg)
  %iface.obj5 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 0
  store ptr %counter, ptr %iface.obj5, align 8
  %iface.method6 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method6, align 8
  %call7 = call i32 @accepts(ptr %iface.tmp4)
  %add = add i32 %call3, %call7
  ret i32 %add
}
