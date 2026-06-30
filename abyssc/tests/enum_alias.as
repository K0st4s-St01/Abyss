type Count = i32;

enum Color {
    Red,
    Green = 4,
    Blue,
}

i32 value(Color color) {
    switch (color) {
        case Red:
            return 1;
        case Green:
            return 4;
        case Blue:
            return 5;
        default:
            return 9;
    }
}

i32 main(i32* argc, str* argv) {
    Count total = value(Blue) + Green;
    return total;
}
