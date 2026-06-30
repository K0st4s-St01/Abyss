; ModuleID = '/home/xator/Abyss/abyssc/tests/for_alias_decl.as'
source_filename = "/home/xator/Abyss/abyssc/tests/for_alias_decl.as"

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %i = alloca i32, align 4
  %total = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 0, ptr %total, align 4
  store i32 1, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %load = load i32, ptr %i, align 4
  %sle = icmp sle i32 %load, 4
  %sle_ext = zext i1 %sle to i32
  %cond = icmp ne i32 %sle_ext, 0
  br i1 %cond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %cur = load i32, ptr %total, align 4
  %load3 = load i32, ptr %i, align 4
  %add = add i32 %cur, %load3
  store i32 %add, ptr %total, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %cur4 = load i32, ptr %i, align 4
  %add5 = add i32 %cur4, 1
  store i32 %add5, ptr %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %load6 = load i32, ptr %total, align 4
  ret i32 %load6
}
