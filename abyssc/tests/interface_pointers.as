interface Readable {
    i32 read();
}

struct Counter {
    i32 value;

    i32 read() {
        return value;
    }
}

Readable* keep(Readable* item) {
    return item;
}

i32 accepts(Readable* item) {
    if (item) {
        return 14;
    }
    return 1;
}

i32 main(i32* argc, str* argv) {
    Counter counter = {9};
    Readable* readable = keep(&counter);
    return accepts(readable) + accepts(&counter);
}
