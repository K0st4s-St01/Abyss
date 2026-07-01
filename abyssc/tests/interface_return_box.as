interface Readable {
    i32 read();
}

struct Counter {
    i32 value;

    i32 read() {
        return value + 1;
    }
}

Readable* as_readable(Counter* counter) {
    return counter;
}

i32 main(i32* argc, str* argv) {
    Counter counter = {41};
    Readable* readable = as_readable(&counter);
    return readable.read();
}
