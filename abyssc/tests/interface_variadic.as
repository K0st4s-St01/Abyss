interface Logger {
    i32 log(str fmt, ...);
}

struct Console {
    i32 value;

    i32 log(str fmt, ...) {
        return value;
    }
}

i32 main(i32* argc, str* argv) {
    Console console = {37};
    Logger* logger = &console;
    return logger.log("first", 1, 2) + logger.log("second") - 37;
}
