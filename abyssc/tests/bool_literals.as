bool invert(bool value) {
    return !value;
}

i32 main(i32* argc, str* argv) {
    bool yes = true;
    bool no = false;

    if (yes && invert(no)) {
        return 12;
    }
    return 1;
}
