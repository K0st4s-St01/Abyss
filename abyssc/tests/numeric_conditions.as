i32 main(i32* argc, str* argv) {
    i32 total = 0;
    i64 big = 0;
    if (!big) {
        total += 5;
    }

    f64 value = 1.5;
    if (value) {
        total += 6;
    }

    while (value) {
        total += 7;
        value = 0.0;
    }

    i32 number = 3;
    i32* ptr = &number;
    if (ptr) {
        total += 3;
    }

    return total;
}
