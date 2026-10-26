#pragma once   // stops this header from being included twice

struct Move {
    int disk;    // 1 = smallest
    int from;    // peg index 0, 1, 2
    int to;
};
