enum Mode {
    Read = 1,
    Write = 2
}

i32 main(i32* argc, str* argv) {
    i32 value = 1;
    switch (value) {
    case Read:
        return 1;
    case 1:
        return 2;
    default:
        return 0;
    }
}
