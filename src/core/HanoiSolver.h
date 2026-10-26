#pragma once
#include "core/Move.h"
#include "core/Config.h"

class HanoiSolver {
public:
    void solve(int diskCount);
    int count() const { return moveCount;}
    const Move& move(int i) const {return moves[i];}

private:
    void solveRec(int n, int from, int to, int aux);
    Move moves[MAX_MOVES]{};
    int moveCount = 0;
};
