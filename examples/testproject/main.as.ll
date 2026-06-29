; ModuleID = '/home/xator/Abyss/examples/testproject/main.as'
source_filename = "/home/xator/Abyss/examples/testproject/main.as"

%Vec2 = type { i32, i32 }
%Rect = type { i32, i32, i32, i32 }

define i32 @Vec2_dot(ptr %self, %Vec2 %other) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %other2 = alloca %Vec2, align 8
  store %Vec2 %other, ptr %other2, align 4
  %ax = alloca i32, align 4
  %deref = load ptr, ptr %self1, align 8
  %x = getelementptr %Vec2, ptr %deref, i32 0, i32 0
  %load = load i32, ptr %x, align 4
  store i32 %load, ptr %ax, align 4
  %ay = alloca i32, align 4
  %deref3 = load ptr, ptr %self1, align 8
  %y = getelementptr %Vec2, ptr %deref3, i32 0, i32 1
  %load4 = load i32, ptr %y, align 4
  store i32 %load4, ptr %ay, align 4
  %bx = alloca i32, align 4
  %x5 = getelementptr %Vec2, ptr %other2, i32 0, i32 0
  %load6 = load i32, ptr %x5, align 4
  store i32 %load6, ptr %bx, align 4
  %by = alloca i32, align 4
  %y7 = getelementptr %Vec2, ptr %other2, i32 0, i32 1
  %load8 = load i32, ptr %y7, align 4
  store i32 %load8, ptr %by, align 4
  %load9 = load i32, ptr %ax, align 4
  %load10 = load i32, ptr %bx, align 4
  %mul = mul i32 %load9, %load10
  %load11 = load i32, ptr %ay, align 4
  %load12 = load i32, ptr %by, align 4
  %mul13 = mul i32 %load11, %load12
  %add = add i32 %mul, %mul13
  ret i32 %add
}

define i32 @Vec2_len_sq(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %deref = load ptr, ptr %self1, align 8
  %x = getelementptr %Vec2, ptr %deref, i32 0, i32 0
  %deref2 = load ptr, ptr %self1, align 8
  %x3 = getelementptr %Vec2, ptr %deref2, i32 0, i32 0
  %load = load i32, ptr %x, align 4
  %load4 = load i32, ptr %x3, align 4
  %mul = mul i32 %load, %load4
  %deref5 = load ptr, ptr %self1, align 8
  %y = getelementptr %Vec2, ptr %deref5, i32 0, i32 1
  %deref6 = load ptr, ptr %self1, align 8
  %y7 = getelementptr %Vec2, ptr %deref6, i32 0, i32 1
  %load8 = load i32, ptr %y, align 4
  %load9 = load i32, ptr %y7, align 4
  %mul10 = mul i32 %load8, %load9
  %add = add i32 %mul, %mul10
  ret i32 %add
}

define i32 @Rect_area(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %deref = load ptr, ptr %self1, align 8
  %w = getelementptr %Rect, ptr %deref, i32 0, i32 2
  %deref2 = load ptr, ptr %self1, align 8
  %h = getelementptr %Rect, ptr %deref2, i32 0, i32 3
  %load = load i32, ptr %w, align 4
  %load3 = load i32, ptr %h, align 4
  %mul = mul i32 %load, %load3
  ret i32 %mul
}

define i32 @Rect_perimeter(ptr %self) {
entry:
  %self1 = alloca ptr, align 8
  store ptr %self, ptr %self1, align 8
  %w = alloca i32, align 4
  %deref = load ptr, ptr %self1, align 8
  %w2 = getelementptr %Rect, ptr %deref, i32 0, i32 2
  %load = load i32, ptr %w2, align 4
  store i32 %load, ptr %w, align 4
  %h = alloca i32, align 4
  %deref3 = load ptr, ptr %self1, align 8
  %h4 = getelementptr %Rect, ptr %deref3, i32 0, i32 3
  %load5 = load i32, ptr %h4, align 4
  store i32 %load5, ptr %h, align 4
  %load6 = load i32, ptr %w, align 4
  %load7 = load i32, ptr %w, align 4
  %add = add i32 %load6, %load7
  %load8 = load i32, ptr %h, align 4
  %add9 = add i32 %add, %load8
  %load10 = load i32, ptr %h, align 4
  %add11 = add i32 %add9, %load10
  ret i32 %add11
}

define i32 @main(ptr %argc, ptr %argv) {
entry:
  %argc1 = alloca ptr, align 8
  store ptr %argc, ptr %argc1, align 8
  %argv2 = alloca ptr, align 8
  store ptr %argv, ptr %argv2, align 8
  %a = alloca %Vec2, align 8
  %x = getelementptr %Vec2, ptr %a, i32 0, i32 0
  %x3 = getelementptr %Vec2, ptr %a, i32 0, i32 0
  store i32 3, ptr %x3, align 4
  %y = getelementptr %Vec2, ptr %a, i32 0, i32 1
  %y4 = getelementptr %Vec2, ptr %a, i32 0, i32 1
  store i32 4, ptr %y4, align 4
  %b = alloca %Vec2, align 8
  %x5 = getelementptr %Vec2, ptr %b, i32 0, i32 0
  %x6 = getelementptr %Vec2, ptr %b, i32 0, i32 0
  store i32 1, ptr %x6, align 4
  %y7 = getelementptr %Vec2, ptr %b, i32 0, i32 1
  %y8 = getelementptr %Vec2, ptr %b, i32 0, i32 1
  store i32 2, ptr %y8, align 4
  %dot = alloca i32, align 4
  %load = load %Vec2, ptr %b, align 4
  %call = call i32 @Vec2_dot(ptr %a, %Vec2 %load)
  store i32 %call, ptr %dot, align 4
  %len_sq = alloca i32, align 4
  %call9 = call i32 @Vec2_len_sq(ptr %a)
  store i32 %call9, ptr %len_sq, align 4
  %r = alloca %Rect, align 8
  %x10 = getelementptr %Rect, ptr %r, i32 0, i32 0
  %x11 = getelementptr %Rect, ptr %r, i32 0, i32 0
  store i32 0, ptr %x11, align 4
  %y12 = getelementptr %Rect, ptr %r, i32 0, i32 1
  %y13 = getelementptr %Rect, ptr %r, i32 0, i32 1
  store i32 0, ptr %y13, align 4
  %w = getelementptr %Rect, ptr %r, i32 0, i32 2
  %w14 = getelementptr %Rect, ptr %r, i32 0, i32 2
  store i32 10, ptr %w14, align 4
  %h = getelementptr %Rect, ptr %r, i32 0, i32 3
  %h15 = getelementptr %Rect, ptr %r, i32 0, i32 3
  store i32 5, ptr %h15, align 4
  %area = alloca i32, align 4
  %call16 = call i32 @Rect_area(ptr %r)
  store i32 %call16, ptr %area, align 4
  %peri = alloca i32, align 4
  %call17 = call i32 @Rect_perimeter(ptr %r)
  store i32 %call17, ptr %peri, align 4
  %result = alloca i32, align 4
  %load18 = load i32, ptr %dot, align 4
  %load19 = load i32, ptr %len_sq, align 4
  %add = add i32 %load18, %load19
  %load20 = load i32, ptr %area, align 4
  %add21 = add i32 %add, %load20
  %load22 = load i32, ptr %peri, align 4
  %add23 = add i32 %add21, %load22
  store i32 %add23, ptr %result, align 4
  %load24 = load i32, ptr %result, align 4
  ret i32 %load24
}
