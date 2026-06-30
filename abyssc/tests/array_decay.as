struct Bag {
    i32 values[2];
}

i32 global_values[3] = {2, 4, 6};

i32 sum3(i32* values) {
    return values[0] + values[1] + values[2];
}

i32 sum2(i32* values) {
    return values[0] + values[1];
}

i32 main(i32* argc, str* argv) {
    i32 local_values[3] = {3, 5, 7};
    Bag bag;
    bag.values[0] = 11;
    bag.values[1] = 13;

    return sum3(local_values) + sum3(global_values) + sum2(bag.values);
}
