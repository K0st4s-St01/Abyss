str empty_ptr() {
    return null;
}

i32 main(i32* argc, str* argv) {
    str s = "abyss";
    str missing = empty_ptr();
    i32 total = 0;
    if (s) {
        total += 10;
    }
    if (!missing) {
        total += 11;
    }
    if (missing == null) {
        total += 12;
    }
    if (s != null) {
        total += 9;
    }
    return total;
}
