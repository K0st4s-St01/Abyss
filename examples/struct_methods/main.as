struct Point {
    i32 x;
    i32 y;

    i32 length_sq() {
        return self.x * self.x + self.y * self.y;
    }

    i32 dot(Point other) {
        return self.x * other.x + self.y * other.y;
    }

    static Point make(i32 x, i32 y) {
        Point p = {x, y};
        return p;
    }
}

i32 main(i32* argc, str* argv) {
    Point a = Point.make(3, 4);
    Point b = Point.make(1, 2);
    return a.length_sq() + a.dot(b);
}
