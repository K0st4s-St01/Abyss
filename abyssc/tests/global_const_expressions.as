enum Flags {
    Base = 3
}

i32 arithmetic = 4 + 5 * 2;
i32 shifted = (Base + 1) << 2;
i32 masked = ((Base + 1) << 2) - 4;
i32 inverted = ~-1;
f64 ratio = 9.0 / 2.0;

i32 main(i32* argc, str* argv) {
    if (ratio > 4.0) {
        return arithmetic + masked + inverted;
    }
    return 1;
}
