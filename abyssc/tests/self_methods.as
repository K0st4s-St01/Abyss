struct Counter {
    i32 value;

    i32 bump(i32 amount) {
        self.value += amount;
        return self.value;
    }

    i32 read() {
        return self.value;
    }

    i32 bump_twice(i32 amount) {
        self.bump(amount);
        return self.bump(amount);
    }
}

i32 main(i32* argc, str* argv) {
    Counter counter;
    Counter* ptr = &counter;
    counter.value = 10;
    return counter.bump(3) + ptr.bump_twice(2) + counter.read();
}
