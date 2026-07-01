struct Box {
    i32 value;

    i32 read() {
        return value;
    }
}

i32 main(i32* argc, str* argv) {
    Box box = {1};
    box.read = 2;
    return box.read();
}
