struct Box {
    i32 value;
}

i32 main(i32* argc, str* argv) {
    i32 value = 1;
    value <<= 3;
    value >>= 1;

    i32 nums[2] = {2, 4};
    nums[0] <<= 2;

    Box box = {3};
    box.value <<= 2;

    i32* ptr = &value;
    *ptr <<= 1;

    return value + nums[0] + box.value;
}
