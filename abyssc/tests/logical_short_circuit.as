i32 mark(i32* slot) {
    *slot = 1;
    return 1;
}

i32 main(i32* argc, str* argv) {
    i32 hit = 0;

    if (0 && mark(&hit)) {
        return 1;
    }
    if (hit != 0) {
        return 2;
    }

    if (1 || mark(&hit)) {
        if (hit == 0) {
            return 0;
        }
        return 3;
    }

    return 4;
}
