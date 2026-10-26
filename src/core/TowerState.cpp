#include "core/TowerState.h"

TowerState::TowerState(int diskCount) {
    // Limitar diskCount al rango [0, MAX_DISKS] para no escribir fuera del arreglo
    if (diskCount > MAX_DISKS) {
        diskCount = MAX_DISKS;
    } else if (diskCount < 0) {
        diskCount = 0;
    }

    // Apilar diskCount, diskCount-1, ..., 1 en la torre 0 (el mayor queda abajo, en el índice 0)
    for (int disk = diskCount; disk >= 1; disk--) {
        pegs[0][count[0]] = disk;
        count[0]++;
    }
}

bool TowerState::canMove(int from, int to) const {
    // No hay disco que mover
    if (count[from] == 0) {
        return false;
    }
    // Cualquier disco puede ir a una torre vacía
    if (count[to] == 0) {
        return true;
    }
    // Solo se permite poner un disco menor encima de uno mayor
    int movingDisk = pegs[from][count[from] - 1];
    int targetTop = pegs[to][count[to] - 1];
    return movingDisk < targetTop;
}

bool TowerState::apply(const Move &m) {
    if (!canMove(m.from, m.to)) {
        return false;
    }

    // Pop de la torre origen
    count[m.from]--;
    int disk = pegs[m.from][count[m.from]];

    // Push en la torre destino
    pegs[m.to][count[m.to]] = disk;
    count[m.to]++;

    return true;
}
