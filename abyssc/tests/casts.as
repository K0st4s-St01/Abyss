i32 main(i32* argc, str* argv) {
    f64 value = 12.75;
    i32 whole = (i32)value;
    f64 again = (f64)whole + 1.5;

    if (again > 13.0) {
        return whole + 1;
    }
    return 1;
}
