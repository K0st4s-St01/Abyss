i64 pick_i64(i64 value) {
    if (value == 3) {
        return value + 4;
    }
    return 0;
}

void* none() {
    return null;
}

i32 main(i32* argc, str* argv) {
    void* p = null;
    if (none() == null) {
        if (p == null) {
            i64 result = pick_i64(3);
            if (result == 7) {
                return 42;
            }
        }
    }
    return 1;
}
