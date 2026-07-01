struct Point {
    i32 x;
    i32 y;
}

struct Grid {
    Point points[2];
    i32 weights[3];
}

Point global_points[2] = {{1, 2}, {3, 4}};
Grid global_grid = {{{5, 6}, {7, 8}}, {9, 10}};

i32 main(i32* argc, str* argv) {
    Point local_points[2] = {{11, 12}, {13, 14}};
    Grid local_grid = {{{15, 16}, {17, 18}}, {19, 20, 21}};

    return global_points[0].x + global_points[1].y +
        global_grid.points[0].x + global_grid.points[1].y +
        global_grid.weights[0] + global_grid.weights[2] +
        local_points[0].x + local_points[1].y +
        local_grid.points[0].x + local_grid.points[1].y +
        local_grid.weights[0] + local_grid.weights[2];
}
