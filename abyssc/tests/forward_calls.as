i32 main(i32* argc, str* argv) {
    Count x = make();
    return later(4) + x;
}

Count make() {
    return 14;
}

type Count = i32;

i32 later(i32 value) {
    return value + 8;
}
