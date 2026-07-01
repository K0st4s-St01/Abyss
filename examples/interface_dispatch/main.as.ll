; ModuleID = 'main.as'
source_filename = "main.as"

%Counter = type { i32 }
%Reader = type { ptr, ptr }
%Logger = type { ptr, ptr }

@.str = private constant [8 x i8] c"counter\00"

define i32 @Counter.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}

define i32 @Counter.log(ptr %self, ptr %message, ...) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %message2 = alloca ptr, align 8
  store ptr %message, ptr %message2, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Counter, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  %add = add i32 %load, 1
  ret i32 %add
}

define i32 @consume(ptr %reader) {
entry:
  %reader1 = alloca ptr, align 8
  store ptr %reader, ptr %reader1, align 8
  %load = load ptr, ptr %reader1, align 8
  %iface.obj = getelementptr inbounds nuw %Reader, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj, align 8
  %iface.fn = getelementptr inbounds nuw %Reader, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 %iface.fn.load(ptr %iface.obj.load)
  ret i32 %iface.call
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp3 = alloca %Logger, align 8
  %logger = alloca ptr, align 8
  %iface.tmp = alloca %Reader, align 8
  %reader = alloca ptr, align 8
  %counter = alloca %Counter, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Counter zeroinitializer, ptr %counter, align 4
  %struct.init = getelementptr %Counter, ptr %counter, i32 0, i32 0
  store i32 41, ptr %struct.init, align 4
  %iface.obj = getelementptr inbounds nuw %Reader, ptr %iface.tmp, i32 0, i32 0
  store ptr %counter, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Reader, ptr %iface.tmp, i32 0, i32 1
  store ptr @Counter.read, ptr %iface.method, align 8
  store ptr %iface.tmp, ptr %reader, align 8
  %iface.obj4 = getelementptr inbounds nuw %Logger, ptr %iface.tmp3, i32 0, i32 0
  store ptr %counter, ptr %iface.obj4, align 8
  %iface.method5 = getelementptr inbounds nuw %Logger, ptr %iface.tmp3, i32 0, i32 1
  store ptr @Counter.log, ptr %iface.method5, align 8
  store ptr %iface.tmp3, ptr %logger, align 8
  %arg = load ptr, ptr %reader, align 8
  %call = call i32 @consume(ptr %arg)
  %load = load ptr, ptr %logger, align 8
  %iface.obj6 = getelementptr inbounds nuw %Logger, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj6, align 8
  %iface.fn = getelementptr inbounds nuw %Logger, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 (ptr, ptr, ...) %iface.fn.load(ptr %iface.obj.load, ptr @.str, i32 1, i32 2)
  %add = add i32 %call, %iface.call
  %sub = sub i32 %add, 41
  ret i32 %sub
}
