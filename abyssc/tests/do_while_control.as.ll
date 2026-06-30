; ModuleID = '/home/xator/Abyss/abyssc/tests/do_while_control.as'
source_filename = "/home/xator/Abyss/abyssc/tests/do_while_control.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %total = alloca i32, align 4
  %count = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %count, align 4
  store i32 0, ptr %total, align 4
  br label %dowhile.body

dowhile.body:                                     ; preds = %dowhile.cond, %entry
  %cur = load i32, ptr %count, align 4
  %add = add i32 %cur, 1
  store i32 %add, ptr %count, align 4
  %load = load i32, ptr %count, align 4
  %eq = icmp eq i32 %load, 2
  %eq_ext = zext i1 %eq to i32
  %cond = icmp ne i32 %eq_ext, 0
  br i1 %cond, label %if.then, label %if.end

dowhile.cond:                                     ; preds = %if.end11, %if.then
  br i1 true, label %dowhile.body, label %dowhile.end

dowhile.end:                                      ; preds = %dowhile.cond, %if.then10
  %load12 = load i32, ptr %total, align 4
  ret i32 %load12

if.then:                                          ; preds = %dowhile.body
  br label %dowhile.cond

if.end:                                           ; preds = %dowhile.body
  %cur3 = load i32, ptr %total, align 4
  %load4 = load i32, ptr %count, align 4
  %add5 = add i32 %cur3, %load4
  store i32 %add5, ptr %total, align 4
  %load6 = load i32, ptr %count, align 4
  %eq7 = icmp eq i32 %load6, 4
  %eq_ext8 = zext i1 %eq7 to i32
  %cond9 = icmp ne i32 %eq_ext8, 0
  br i1 %cond9, label %if.then10, label %if.end11

if.then10:                                        ; preds = %if.end
  br label %dowhile.end

if.end11:                                         ; preds = %if.end
  br label %dowhile.cond
}
