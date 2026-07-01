struct Counter {
    i32 value;

    i32 get() {
        return value;
    }

    i32 get() {
        return value + 1;
    }
}

i32 main(i32* argc, str* argv) {
    Counter c = {1};
    return c.get();
}
