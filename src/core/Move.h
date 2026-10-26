#pragma once   // stops this header from being included twice

// One step of the solution. Plain data with no behaviour, so it's easy to store in an array.
struct Move {
    int disk;    // 1 = smallest
    int from;    // peg index 0, 1, 2
    int to;
};
