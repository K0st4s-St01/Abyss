import left;
import right;

i32 main(i32* argc, str* argv) {
    left.Point lp = {1};
    right.Point rp = {2};
    return lp.x + rp.x + left.answer() + right.answer();
}
