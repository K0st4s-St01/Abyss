import left;
import right;

interface Readable {
    i32 read();
}

i32 main(i32* argc, str* argv) {
    left.Counter lc = {1};
    right.Counter rc = {2};
    Readable* lr = &lc;
    Readable* rr = &rc;
    return lr.read() + rr.read() + left.answer() + right.answer();
}
