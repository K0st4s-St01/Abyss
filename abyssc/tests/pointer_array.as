i32 main(i32* argc, str* argv) {
    i32* xs = new i32[3];
    xs[0] = 10;
    xs[1] = 20;
    xs[2] = 30;
    i32 result = xs[0] + xs[1] + xs[2];
    delete[] xs;
    return result;
}
