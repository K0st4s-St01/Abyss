type Count = i16;

i64 global_size_source;

i32 main(i32* argc, str* argv) {
    i32 local_size_source = 0;
    return sizeof(i8) + sizeof(Count) + sizeof(i32*) +
           sizeof(global_size_source) + sizeof(local_size_source);
}
