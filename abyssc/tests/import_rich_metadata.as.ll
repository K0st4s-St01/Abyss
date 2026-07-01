; ModuleID = '/home/xator/Abyss/abyssc/tests/import_rich_metadata.as'
source_filename = "/home/xator/Abyss/abyssc/tests/import_rich_metadata.as"

%Box = type { i32 }
%Readable = type { ptr, ptr }
%Point = type { i32, i32 }

@meta.imported_counter = external global i32
@imported_counter = external global i32

define i32 @Box.read(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %self_field_base = load ptr, ptr %self1, align 8
  %value = getelementptr %Box, ptr %self_field_base, i32 0, i32 0
  %load = load i32, ptr %value, align 4
  ret i32 %load
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %iface.tmp = alloca %Readable, align 8
  %r = alloca ptr, align 8
  %b = alloca %Box, align 8
  %p = alloca %Point, align 8
  %n = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 5, ptr %n, align 4
  store %Point zeroinitializer, ptr %p, align 4
  %struct.init = getelementptr %Point, ptr %p, i32 0, i32 0
  store i32 2, ptr %struct.init, align 4
  %struct.init3 = getelementptr %Point, ptr %p, i32 0, i32 1
  store i32 3, ptr %struct.init3, align 4
  store %Box zeroinitializer, ptr %b, align 4
  %struct.init4 = getelementptr %Box, ptr %b, i32 0, i32 0
  store i32 4, ptr %struct.init4, align 4
  %iface.obj = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 0
  store ptr %b, ptr %iface.obj, align 8
  %iface.method = getelementptr inbounds nuw %Readable, ptr %iface.tmp, i32 0, i32 1
  store ptr @Box.read, ptr %iface.method, align 8
  store ptr %iface.tmp, ptr %r, align 8
  %load = load ptr, ptr %r, align 8
  %cond = icmp ne ptr %load, null
  br i1 %cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %load5 = load i32, ptr %n, align 4
  %add = add i32 %load5, 7
  %x = getelementptr %Point, ptr %p, i32 0, i32 0
  %load6 = load i32, ptr %x, align 4
  %add7 = add i32 %add, %load6
  %load8 = load i32, ptr @imported_counter, align 4
  %add9 = add i32 %add7, %load8
  ret i32 %add9

if.end:                                           ; preds = %entry
  ret i32 1
}
