type Count = i32;

i32 main(i32* argc, str* argv) {
    i32 total = 0;
    for (Count i = 1; i <= 4; i += 1) {
        total += i;
    }
    return total;
}
