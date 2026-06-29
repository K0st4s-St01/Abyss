i32 classify(i32 x) {
    i32 result = 0;
    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        case 3:
            return 30;
        default:
            result = 40;
            break;
    }
    return result;
}

i32 main(i32* argc, str* argv) {
    i32 a = classify(2);
    i32 b = classify(3);
    i32 c = classify(9);
    return a + b + c;
}
