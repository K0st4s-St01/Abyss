interface Reader {
    i32 read();
}

interface Logger {
    i32 log(str message, ...);
}

struct Counter {
    i32 value;

    i32 read() {
        return value;
    }

    i32 log(str message, ...) {
        return value + 1;
    }
}

i32 consume(Reader* reader) {
    return reader.read();
}

i32 main(i32* argc, str* argv) {
    Counter counter = {41};
    Reader* reader = &counter;
    Logger* logger = &counter;
    return consume(reader) + logger.log("counter", 1, 2) - 41;
}
