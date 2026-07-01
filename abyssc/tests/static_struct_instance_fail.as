struct Box {
    i32 value;

    static i32 base() {
        return 20;
    }
}

i32 main(i32* argc, str* argv) {
    Box box = {2};
    return box.base();
}
