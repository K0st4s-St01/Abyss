i32 use_point(Point p) {
    return p.x + Blue;
}

i32 accepts(Readable* item) {
    if (item) {
        return 5;
    }
    return 1;
}

i32 main(i32* argc, str* argv) {
    Count base = 2;
    Point p = {base, 4};
    Counter counter = {6};
    return use_point(p) + accepts(&counter);
}

enum Color {
    Red = 1,
    Blue = 3,
}

interface Readable {
    i32 read();
}

struct Point {
    Count x;
    i32 y;
}

type Count = i32;

struct Counter {
    i32 value;

    i32 read() {
        return value;
    }
}
