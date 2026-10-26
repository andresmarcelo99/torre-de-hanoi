#include "core/HanoiSolver.h"

void HanoiSolver::solve(int diskCount) {
    if (diskCount > MAX_DISKS) {
        diskCount = MAX_DISKS;
    } else if (diskCount < 0) {
        diskCount = 0;
    }

    moveCount = 0;
    // Llamada inicial: mover todos los discos de la torre 0 a la 2, usando la 1 como auxiliar
    solveRec(diskCount, 0, 2, 1);
}

void HanoiSolver::solveRec(int n, int from, int to, int aux) {
    if (n > 0) {
        // Este paso quita la sub-torre y la mueve al aux
        solveRec(n - 1, from, aux, to);
        moves[moveCount] = {.disk = n, .from = from, .to = to};
        moveCount++;
        //Este paso construye la sub-torre encima del disco mayor ubicado en el destino final(int to)
        solveRec(n - 1, aux, to ,from);
    }
}
