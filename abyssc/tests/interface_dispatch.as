interface Readable {
    i32 read();
}

struct Counter {
    i32 value;

    i32 read() {
        return value + 2;
    }
}

i32 take(Readable* item) {
    return item.read();
}

i32 main(i32* argc, str* argv) {
    Counter counter = {40};
    Readable* readable = &counter;
    return readable.read() + take(&counter) - 42;
}
