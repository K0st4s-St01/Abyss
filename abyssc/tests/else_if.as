i32 pick(i32 x) {
    if (x == 1) {
        return 10;
    } else if (x == 2) {
        return 20;
    } else {
        return 30;
    }
}

i32 main(i32* argc, str* argv) {
    return pick(2);
}
