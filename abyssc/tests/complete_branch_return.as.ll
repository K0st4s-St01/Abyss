; ModuleID = '/home/xator/Abyss/abyssc/tests/complete_branch_return.as'
source_filename = "/home/xator/Abyss/abyssc/tests/complete_branch_return.as"

define i32 @choose(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %load = load i32, ptr %value1, align 4
  %eq = icmp eq i32 %load, 0
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  ret i32 10

if.end:                                           ; No predecessors!
  ret i32 0

if.else:                                          ; preds = %entry
  %load2 = load i32, ptr %value1, align 4
  %eq3 = icmp eq i32 %load2, 1
  %eq_ext4 = zext i1 %eq3 to i32
  %cond5 = icmp ne i32 %eq_ext4, 0
  br i1 %cond5, label %elif.then, label %elif.next

elif.then:                                        ; preds = %if.else
  ret i32 20

elif.next:                                        ; preds = %if.else
  ret i32 30
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %call = call i32 @choose(i32 1)
  %call3 = call i32 @choose(i32 2)
  %add = add i32 %call, %call3
  ret i32 %add
}
