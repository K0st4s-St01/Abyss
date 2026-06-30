i32 main(i32* argc, str* argv) {
    i32 count = 0;
    i32 total = 0;

    do {
        count += 1;
        if (count == 2) {
            continue;
        }
        total += count;
        if (count == 4) {
            break;
        }
    } while (true);

    return total;
}
