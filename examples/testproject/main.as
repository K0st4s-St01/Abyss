struct Vec2 {
    i32 x;
    i32 y;
}

i32 Vec2_dot(Vec2* self, Vec2 other) {
    i32 ax = self->x;
    i32 ay = self->y;
    i32 bx = other.x;
    i32 by = other.y;
    return ax * bx + ay * by;
}

i32 Vec2_len_sq(Vec2* self) {
    return self->x * self->x + self->y * self->y;
}

struct Rect {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
}

i32 Rect_area(Rect* self) {
    return self->w * self->h;
}

i32 Rect_perimeter(Rect* self) {
    i32 w = self->w;
    i32 h = self->h;
    return w + w + h + h;
}

i32 main(i32* argc, str* argv) {
    Vec2 a;
    a.x = 3;
    a.y = 4;

    Vec2 b;
    b.x = 1;
    b.y = 2;

    i32 dot = Vec2_dot(&a, b);
    i32 len_sq = Vec2_len_sq(&a);

    Rect r;
    r.x = 0;
    r.y = 0;
    r.w = 10;
    r.h = 5;

    i32 area = Rect_area(&r);
    i32 peri = Rect_perimeter(&r);

    i32 result = dot + len_sq + area + peri;
    return result;
}
