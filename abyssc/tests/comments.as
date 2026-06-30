// leading line comment
static i32 value = 3; /* trailing block comment */

/*
   multi-line block comment with tokens that should be ignored:
   i32 broken = ;
*/
i32 main(i32* argc, str* argv) {
    value += 4; // statement comment
    # legacy hash comment still works
    return value;
}
