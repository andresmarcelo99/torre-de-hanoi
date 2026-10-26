#include "app/CallStack.h"

// solveRec(n, from, to, aux) hace sus movimientos siempre en este orden:
//   los 2^(n-1) - 1 de su 1a llamada, después el suyo, después los 2^(n-1) - 1 de su 2a llamada.
// Entonces, con el número de movimiento sabemos en qué parte de cada llamada estamos,
// y bajamos por la recursión sin ejecutarla.
int callStackAt(int diskCount, int move, Frame frames[MAX_DISKS]) {
    int n = diskCount, from = 0, to = 2, aux = 1;   // la llamada inicial que hace solve()
    int i = move;                                   // el movimiento buscado, contado dentro de esta llamada
    int depth = 0;

    while (n > 0) {
        int half = (1 << (n - 1)) - 1;              // movimientos de cada llamada recursiva
        if (i < half) {
            // Está dentro de la 1a llamada: solveRec(n - 1, from, aux, to)
            frames[depth++] = {n, from, to, aux, 1};
            int oldTo = to;
            to = aux;
            aux = oldTo;
        } else if (i == half) {
            // Es el movimiento de esta misma llamada: es la cima de la pila
            frames[depth++] = {n, from, to, aux, 0};
            return depth;
        } else {
            // Está dentro de la 2a llamada: solveRec(n - 1, aux, to, from)
            frames[depth++] = {n, from, to, aux, 2};
            i -= half + 1;                          // saltear la 1a llamada y el movimiento propio
            int oldFrom = from;
            from = aux;
            aux = oldFrom;
        }
        n--;
    }
    return depth;
}

// Cada llamada con n > 0 hace exactamente un movimiento, y si el árbol de llamadas se recorre
// en orden (hijo izq, nodo, hijo der) los nodos salen en el mismo orden que los movimientos.
// Así que la llamada k es la que hace el movimiento k, y su sub-árbol hace los movimientos
// k - h .. k + h, con h = 2^(disk-1) - 1.
int callStatus(int k, int disk, int current) {
    int h = (1 << (disk - 1)) - 1;
    if (k + h < current) return -1;   // todo su sub-árbol ya se movió: retornó
    if (k - h > current) return 1;    // nada de su sub-árbol se movió todavía: no se llamó
    return 0;                         // 'current' cae dentro de su sub-árbol: está en la pila
}
