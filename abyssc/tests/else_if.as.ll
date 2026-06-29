; ModuleID = '/home/xator/Abyss/abyssc/tests/else_if.as'
source_filename = "/home/xator/Abyss/abyssc/tests/else_if.as"

define i32 @pick(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %load = load i32, ptr %x1, align 4
  %eq = icmp eq i32 %load, 1
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  ret i32 10

if.end:                                           ; preds = %if.end7
  ret i32 0

if.else:                                          ; preds = %entry
  %load2 = load i32, ptr %x1, align 4
  %eq3 = icmp eq i32 %load2, 2
  %eq_ext4 = zext i1 %eq3 to i32
  %cond5 = icmp ne i32 %eq_ext4, 0
  br i1 %cond5, label %if.then6, label %if.else8

if.then6:                                         ; preds = %if.else
  ret i32 20

if.end7:                                          ; No predecessors!
  br label %if.end

if.else8:                                         ; preds = %if.else
  ret i32 30
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @pick(i32 2)
  ret i32 %call
}
