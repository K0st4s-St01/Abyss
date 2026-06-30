i32 main(i32* argc, str* argv) {
    i32 i = 0;
    i++;
    ++i;
    i--;
    --i;

    i32 total = 0;
    for (i32 j = 0; j < 3; j++) {
        total += j;
    }

    return i + total;
}
