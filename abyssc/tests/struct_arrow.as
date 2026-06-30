struct Point {
    i32 x;
    i32 y;
}

i32 sum(Point* point) {
    return point->x + point->y;
}

i32 main(i32* argc, str* argv) {
    Point p;
    p.x = 2;
    p.y = 5;
    return sum(&p);
}
