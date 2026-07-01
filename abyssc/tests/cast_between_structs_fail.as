struct Point {
    i32 x;
}

struct Other {
    i32 x;
}

i32 main(i32* argc, str* argv) {
    Point p = {1};
    Other o = (Other)p;
    return o.x;
}
