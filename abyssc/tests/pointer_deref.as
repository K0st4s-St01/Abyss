i32 store(i32* slot) {
    *slot = 7;
    return 0;
}

i32 main(i32* argc, str* argv) {
    i32 value = 0;
    store(&value);
    return value;
}
