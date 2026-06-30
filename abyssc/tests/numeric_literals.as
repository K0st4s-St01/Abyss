enum Bits {
    Hex = 0x10,
    Bin = 0b1010,
    Oct = 0o7,
}

i32 main(i32* argc, str* argv) {
    i32 decimal = 1_000;
    i32 binary = 0b1_010;
    i32 hex = 0x0f;
    i32 octal = 0o10;
    f64 value = 2_5.0;
    return Hex + Bin + Oct + (decimal / 100) + binary + hex + octal +
           (i32)(value / 5.0);
}
