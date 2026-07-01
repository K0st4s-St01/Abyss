u64 half64 = 0xffffffffffffffff / 2;
u64 rem64 = 0xffffffffffffffff % 2;
u64 shifted64 = 0xffffffffffffffff >> 63;
u32 half32 = 0xffffffff / 2;
u32 rem32 = 0xffffffff % 2;
u32 shifted32 = 0xffffffff >> 31;

i32 main(i32* argc, str* argv) {
    i32 total = 0;
    if (half64 > 1) {
        total += 10;
    }
    if (rem64 == 1) {
        total += 2;
    }
    if (shifted64 == 1) {
        total += 3;
    }
    if (half32 > 1) {
        total += 4;
    }
    if (rem32 == 1) {
        total += 5;
    }
    if (shifted32 == 1) {
        total += 6;
    }
    return total;
}
