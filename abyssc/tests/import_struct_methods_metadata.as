import impl;

interface Readable {
    i32 read();
}

i32 main(i32* argc, str* argv) {
    Counter counter = {40};
    Readable* readable = &counter;
    return readable.read();
}
