; ModuleID = '/home/xator/Abyss/abyssc/tests/import_struct_methods_metadata.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_struct_methods_metadata.as"

%Readable = type { ptr, ptr }
%Counter = type { i32 }

declare i32 @impl.Counter.read(ptr)

declare i32 @Counter.read(ptr)

define i32 @main(ptr %argc, ptr %argv) {
entry:
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
  ret i32 %iface.call
}
