i32 choose(i32 value) {
    if (value == 0) {
        return 10;
    } elif (value == 1) {
        return 20;
    } else {
        return 30;
    }
}

i32 main(i32* argc, str* argv) {
    return choose(1) + choose(2);
}
