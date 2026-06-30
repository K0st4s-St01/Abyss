f64 scale(f64 value) {
    return value * 2.0;
}

i32 main(i32* argc, str* argv) {
    f64 total = scale(1.5);
    total += 1;

    f32 small = 2.5;
    f64 mixed = total + small;

    if (mixed >= 6.5) {
        if (-small < 0.0) {
            return 42;
        }
    }
    return 1;
}
