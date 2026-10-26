#pragma once
#include "core/Config.h"

// One active call of solveRec, as it sits on the call stack.
struct Frame {
    int n, from, to, aux;   // the call's arguments
    int phase;              // 1 = inside the 1st recursive call, 0 = making its move, 2 = inside the 2nd
};

// Rebuilds solveRec's call stack at the moment it records move 'move' (0-based) for diskCount
// disks, without running solveRec. Writes frames[0 .. depth-1] (frames[0] = the first call,
// frames[depth-1] = the call making the move) and returns depth.
int callStackAt(int diskCount, int move, Frame frames[MAX_DISKS]);

// State of the call that makes move k (moving disk 'disk') when move 'current' is next:
// -1 = already returned, 0 = on the stack, 1 = not called yet.
int callStatus(int k, int disk, int current);
