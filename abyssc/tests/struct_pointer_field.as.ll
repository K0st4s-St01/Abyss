; ModuleID = '/home/xator/Abyss/abyssc/tests/struct_pointer_field.as'
source_filename = "/home/xator/Abyss/abyssc/tests/struct_pointer_field.as"

%Holder = type { ptr }

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %holder = alloca %Holder, align 8
  %number = alloca i32, align 4
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  store i32 33, ptr %number, align 4
  %value = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %value3 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  store ptr %number, ptr %value3, align 8
  %value4 = getelementptr %Holder, ptr %holder, i32 0, i32 0
  %load = load ptr, ptr %value4, align 8
  %load5 = load i32, ptr %load, align 4
  ret i32 %load5
}
