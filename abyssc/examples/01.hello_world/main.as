
struct Point {
    i32 x;
    i32 y;

    i32 distance_sq(Point* self, Point other) {
        i32 dx = self->x - other.x;
        i32 dy = self->y - other.y;
        return dx * dx + dy * dy;
    }
}

i32 main(i32* argc, str* argv) {
    return 0;
}
