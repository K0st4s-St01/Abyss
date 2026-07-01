interface Logger {
    i32 log(str fmt, ...);
}

struct Console {
    i32 log(str fmt, ...) {
        return 1;
    }
}

i32 main(i32* argc, str* argv) {
    Console console = {};
    Logger* logger = &console;
    return logger.log();
}
