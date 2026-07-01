i32 main(i32* argc, str* argv) {
    void* value = new MissingType[1];
    delete[] value;
    return 0;
}
