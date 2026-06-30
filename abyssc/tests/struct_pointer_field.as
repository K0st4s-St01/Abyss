struct Holder {
    i32* value;
}

i32 main(i32* argc, str* argv) {
    i32 number = 33;
    Holder holder;
    holder.value = &number;
    return *holder.value;
}
