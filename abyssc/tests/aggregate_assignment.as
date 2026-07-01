struct Point {
    i32 x;
    i32 y;
}

struct Holder {
    Point points[2];
    i32 weights[3];
}

i32 main(i32* argc, str* argv) {
    Point point = {1, 2};
    point = {3, 4};

    Holder holder;
    holder.points[0] = {5, 6};
    holder.points[1] = {7, 8};
    holder.weights = {9, 10, 11};

    return point.x + point.y +
        holder.points[0].x + holder.points[0].y +
        holder.points[1].x + holder.points[1].y +
        holder.weights[0] + holder.weights[1] + holder.weights[2];
}
