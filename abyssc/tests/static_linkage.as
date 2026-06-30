static i32 seed = 9;

static i32 add_seed(i32 value) {
    seed += value;
    return seed;
}

i32 main(i32* argc, str* argv) {
    return add_seed(4) + seed;
}
