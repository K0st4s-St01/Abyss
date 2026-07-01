i32 maybe(i32 value) {
    if (value > 0) {
        return value;
    }
}

i32 main(i32* argc, str* argv) {
    return maybe(1);
}
