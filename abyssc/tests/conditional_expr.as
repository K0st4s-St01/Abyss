i32 choose(i32 value) {
    return value > 10 ? value : 10;
}

i32 nested(i32 value) {
    return value < 0 ? 1 : value == 0 ? 2 : 3;
}

i32 main(i32* argc, str* argv) {
    i32 a = choose(4);
    i32 b = choose(15);
    i32 c = nested(-1);
    i32 d = nested(0);
    i32 e = nested(8);
    return a + b + c + d + e;
}
