#pragma once
#include "core/Move.h"
#include "core/Config.h"

// The 3 pegs. Each peg is a stack stored in an array:
// pegs[p][0] is the bottom disk, pegs[p][count[p] - 1] is the top disk.
class TowerState {
public:
    explicit TowerState(int diskCount);

    bool canMove(int from, int to) const;
    bool apply(const Move& m);

    int size(int peg) const { return count[peg];}
    int diskAt(int peg, int level) const { return pegs[peg][level];}

private:
    int pegs[3][MAX_DISKS]{};
    int count[3]{};
};
