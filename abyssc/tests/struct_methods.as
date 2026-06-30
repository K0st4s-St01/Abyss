struct Box {
    i32 value;

    i32 get() {
        return value;
    }

    i32 set(i32 next) {
        value = next;
        value += 0;
        return value;
    }

    i32 add(i32 amount) {
        return value + amount;
    }

    i32 twice() {
        return get() + add(0);
    }
}

i32 main(i32* argc, str* argv) {
    Box box;
    Box* ptr = &box;
    return box.set(5) + ptr.get() + box.add(2) + ptr.twice();
}
