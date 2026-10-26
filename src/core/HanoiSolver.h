#pragma once
#include "core/Move.h"
#include "core/Config.h"

// One active call of solveRec, as it sits on the call stack.
struct Frame {
    int n, from, to, aux;   // the call's arguments
    int phase;              // 1 = inside the 1st recursive call, 0 = making its move, 2 = inside the 2nd
};

class HanoiSolver {
public:
    // Fills moves[0 .. moveCount-1] with the solution for diskCount disks.
    void solve(int diskCount);
    int count() const { return moveCount;}
    const Move& move(int i) const {return moves[i];}

    // Call stack at the moment move i was made: frame(i, 0) is the first call, frame(i, depth - 1) the top.
    int depth(int i) const { return stackDepth[i]; }
    const Frame& frame(int i, int level) const { return stackAt[i][level]; }

private:
    void solveRec(int n, int from, int to, int aux);
    Move moves[MAX_MOVES]{};
    int moveCount = 0;

    // A call with n disks recurses n levels deep, so MAX_DISKS frames are enough.
    Frame stack[MAX_DISKS]{};                 // the calls active right now
    int stackSize = 0;
    Frame stackAt[MAX_MOVES][MAX_DISKS]{};    // a copy of 'stack' saved for every move
    int stackDepth[MAX_MOVES]{};
};
