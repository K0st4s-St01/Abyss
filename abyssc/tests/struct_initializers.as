struct Point {
    i32 x;
    i32 y;
}

struct Pair {
    i32 first;
    i32 second;
    i32 third;
}

Point origin = {1, 2};
Pair global_partial = {6};

i32 main(i32* argc, str* argv) {
    Point p = {7, 9};
    Pair full = {3, 4, 5};
    Pair partial = {8};

    return p.x + p.y + full.first + full.second + full.third +
        partial.first + partial.second + partial.third +
        origin.x + origin.y + global_partial.first +
        global_partial.second + global_partial.third;
}
