import left;

interface Readable {
    i32 read();
}

i32 main(i32* argc, str* argv) {
    Counter counter = {1};
    Readable* readable = &counter;
    return answer() + readable.read();
}
