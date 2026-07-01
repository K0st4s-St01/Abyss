struct Point {
    i32 x;
    i32 y;
}

Point make_point(i32 x, i32 y) {
    return {x, y};
}

i32 sum_point(Point point) {
    return point.x + point.y;
}

i32 main(i32* argc, str* argv) {
    Point made = make_point(7, 8);
    return sum_point({5, 6}) + sum_point(made);
}
