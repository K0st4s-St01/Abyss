i32 main(i32* argc, str* argv) {
    str s = "abyss";
    i32 value = (i32)s[1];
    if ((i32)s[5] == 0) {
        return value - 56;
    }
    return 1;
}
