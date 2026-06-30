i32 read_total(i32 add) {
    return total + add;
}

enum Seed {
    Start = 4,
}

i32 total = Start;
i32 scratch;

i32 main(i32* argc, str* argv) {
    scratch = 6;
    total += scratch;
    return read_total(2) + total + scratch;
}
