; ModuleID = '/home/xator/Abyss/abyssc/tests/numeric_conditions.as'
source_filename = "/home/xator/Abyss/abyssc/tests/numeric_conditions.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %ptr = alloca ptr, align 8
  %number = alloca i32, align 4
  %value = alloca double, align 8
  %big = alloca i64, align 8
  %total = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %total, align 4
  store i64 0, ptr %big, align 4
  %load = load i64, ptr %big, align 4
  %cond = icmp ne i64 %load, 0
  %not = icmp eq i1 %cond, false
  %not_ext = zext i1 %not to i32
  %cond3 = icmp ne i32 %not_ext, 0
  br i1 %cond3, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cur = load i32, ptr %total, align 4
  %add = add i32 %cur, 5
  store i32 %add, ptr %total, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  store double 1.500000e+00, ptr %value, align 8
  %load4 = load double, ptr %value, align 8
  %cond5 = fcmp one double %load4, 0.000000e+00
  br i1 %cond5, label %if.then6, label %if.end7

if.then6:                                         ; preds = %if.end
  %cur8 = load i32, ptr %total, align 4
  %add9 = add i32 %cur8, 6
  store i32 %add9, ptr %total, align 4
  br label %if.end7

if.end7:                                          ; preds = %if.then6, %if.end
  br label %while.cond

while.cond:                                       ; preds = %while.body, %if.end7
  %load10 = load double, ptr %value, align 8
  %cond11 = fcmp one double %load10, 0.000000e+00
  br i1 %cond11, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %cur12 = load i32, ptr %total, align 4
  %add13 = add i32 %cur12, 7
  store i32 %add13, ptr %total, align 4
  store double 0.000000e+00, ptr %value, align 8
  br label %while.cond

while.end:                                        ; preds = %while.cond
  store i32 3, ptr %number, align 4
  store ptr %number, ptr %ptr, align 8
  %load14 = load ptr, ptr %ptr, align 8
  %cond15 = icmp ne ptr %load14, null
  br i1 %cond15, label %if.then16, label %if.end17

if.then16:                                        ; preds = %while.end
  %cur18 = load i32, ptr %total, align 4
  %add19 = add i32 %cur18, 3
  store i32 %add19, ptr %total, align 4
  br label %if.end17

if.end17:                                         ; preds = %if.then16, %while.end
  %load20 = load i32, ptr %total, align 4
  ret i32 %load20
}
