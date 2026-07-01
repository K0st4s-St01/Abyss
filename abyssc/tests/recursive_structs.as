struct Node {
    i32 value;
    Node* next;
}

i32 main(i32* argc, str* argv) {
    Node first = {3, null};
    Node second = {4, &first};
    return second.value + second.next->value;
}
