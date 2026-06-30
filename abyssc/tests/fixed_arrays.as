struct Bag {
    i32 values[3];
}

i32 totals[2] = {12, 4};
i32 partial[3] = {8};

i32 main(i32* argc, str* argv) {
    i32 nums[4] = {5, 7, 0, 3};
    nums[2] = nums[0] + nums[1];

    Bag bag;
    bag.values[0] = totals[0];
    bag.values[1] = totals[1];
    bag.values[2] = 1;

    return nums[2] + nums[3] + partial[0] + partial[1] +
        totals[1] + bag.values[0] + bag.values[1] + bag.values[2];
}
