; ModuleID = '/home/xator/Abyss/abyssc/tests/import_namespace_link.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_namespace_link.as"

%Readable = type { ptr, ptr }
%right.Counter = type { i32 }
%left.Counter = type { i32 }

declare i32 @left.Counter.read(ptr)

declare i32 @left.Counter.base()

declare i32 @Counter.read(ptr)

declare i32 @Counter.base()

declare i32 @left.answer()

declare i32 @answer()

declare i32 @right.Counter.read(ptr)

declare i32 @right.Counter.base()

declare i32 @right.answer()

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp4 = alloca %Readable, align 8
  %rr = alloca ptr, align 8
  %iface.tmp = alloca %Readable, align 8
  %lr = alloca ptr, align 8
  %rc = alloca %right.Counter, align 8
  %lc = alloca %left.Counter, align 8
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store %left.Counter zeroinitializer, ptr %lc, align 4
  %struct.init = getelementptr %left.Counter, ptr %lc, i32 0, i32 0
  store i32 1, ptr %struct.init, align 4
  store %right.Counter zeroinitializer, ptr %rc, align 4
  %struct.init3 = getelementptr %right.Counter, ptr %rc, i32 0, i32 0
  store i32 2, ptr %struct.init3, align 4
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 0
  store ptr %lc, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 1
  store ptr @left.Counter.read, ptr %iface.method, align 8
  store ptr %iface.tmp, ptr %lr, align 8
  %iface.obj5 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 0
  store ptr %rc, ptr %iface.obj5, align 8
  %iface.method6 = getelementptr inbounds nuw %Readable, ptr %iface.tmp4, i32 0, i32 1
  store ptr @right.Counter.read, ptr %iface.method6, align 8
  store ptr %iface.tmp4, ptr %rr, align 8
  %load = load ptr, ptr %lr, align 8
  %iface.obj7 = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 0
  %iface.obj.load = load ptr, ptr %iface.obj7, align 8
  %iface.fn = getelementptr inbounds nuw %Readable, ptr %load, i32 0, i32 1
  %iface.fn.load = load ptr, ptr %iface.fn, align 8
  %iface.call = call i32 %iface.fn.load(ptr %iface.obj.load)
  %load8 = load ptr, ptr %rr, align 8
  %iface.obj9 = getelementptr inbounds nuw %Readable, ptr %load8, i32 0, i32 0
  %iface.obj.load10 = load ptr, ptr %iface.obj9, align 8
  %iface.fn11 = getelementptr inbounds nuw %Readable, ptr %load8, i32 0, i32 1
  %iface.fn.load12 = load ptr, ptr %iface.fn11, align 8
  %iface.call13 = call i32 %iface.fn.load12(ptr %iface.obj.load10)
  %add = add i32 %iface.call, %iface.call13
  %call = call i32 @left.answer()
  %add14 = add i32 %add, %call
  %call15 = call i32 @right.answer()
  %add16 = add i32 %add14, %call15
  ret i32 %add16
}
