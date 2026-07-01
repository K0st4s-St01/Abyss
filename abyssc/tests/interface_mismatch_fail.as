interface Readable {
    i32 read();
}

struct Counter {
    i32 value;
}

i32 accepts(Readable* item) {
    return 0;
}

i32 main(i32* argc, str* argv) {
    Counter counter = {9};
    return accepts(&counter);
}
