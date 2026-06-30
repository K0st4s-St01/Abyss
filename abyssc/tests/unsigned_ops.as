i32 main(i32* argc, str* argv) {
    u32 max = 0xffffffff;
    u32 one = 1;
    u32 two = 2;
    u32 thirty_one = 31;
    u32 half = max / two;
    u32 rem = max % two;
    u32 shifted = max >> thirty_one;

    i32 total = 0;
    if (max > one) {
        total += 10;
    }
    if (half > one) {
        total += 20;
    }
    if (rem == one) {
        total += 3;
    }
    if (shifted == one) {
        total += 4;
    }
    return total;
}
