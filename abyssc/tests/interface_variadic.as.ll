; ModuleID = '/home/xator/Abyss/abyssc/tests/interface_variadic.as'
source_filename = "/home/xator/Abyss/abyssc/tests/interface_variadic.as"

%Console = type { i32 }
%Logger = type { ptr, ptr }

@.str = private constant [6 x i8] c"first\00"
@.str.1 = private constant [7 x i8] c"second\00"

define i32 @Console.log(ptr %self, ptr %fmt, ...) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %fmt2 = alloca ptr, align 8
  store ptr %fmt, ptr %fmt2, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Console, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp = alloca %Logger, align 8
  %logger = alloca ptr, align 8
  %console = alloca %Console, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %Console zeroinitializer, ptr %console, align 4
  %struct.init = getelementptr %Console, ptr %console, i32 0, i32 0
  store i32 37, ptr %struct.init, align 4
  %iface.obj = getelementptr inbounds nuw %Logger, ptr %iface.tmp, i32 0, i32 0
  store ptr %console, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Logger, ptr %iface.tmp, i32 0, i32 1
  store ptr @Console.log, ptr %iface.method, align 8
  store ptr %iface.tmp, ptr %logger, align 8
  %load = load ptr, ptr %logger, align 8
  %iface.obj3 = getelementptr inbounds nuw %Logger, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj3, align 8
  %iface.fn = getelementptr inbounds nuw %Logger, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 (ptr, ptr, ...) %iface.fn.load(ptr %iface.obj.load, ptr @.str, i32 1, i32 2)
  %load4 = load ptr, ptr %logger, align 8
  %iface.obj5 = getelementptr inbounds nuw %Logger, ptr %load4, i32 0, i32 0
  %iface.obj.load6 = load ptr, ptr %iface.obj5, align 8
  %iface.fn7 = getelementptr inbounds nuw %Logger, ptr %load4, i32 0, i32 1
  %iface.fn.load8 = load ptr, ptr %iface.fn7, align 8
  %iface.call9 = call i32 (ptr, ptr, ...) %iface.fn.load8(ptr %iface.obj.load6, ptr @.str.1)
  %add = add i32 %iface.call, %iface.call9
  %sub = sub i32 %add, 37
  ret i32 %sub
}
