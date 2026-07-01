enum Flags {
    Base = 3
}

struct Pair {
    i32 left;
    i32 right;
}

i32 arithmetic = 4 + 5 * 2;
i32 shifted = (Base + 1) << 2;
i32 masked = ((Base + 1) << 2) - 4;
i32 inverted = ~-1;
Pair pair = {4 + 5 * 2, ((Base + 1) << 2) - 4};
i32 values[3] = {4 + 5 * 2, (Base + 1) << 2, ((Base + 1) << 2) - 4};

i32 main(i32* argc, str* argv) {
    return pair.left + pair.right + values[1] + inverted;
}
