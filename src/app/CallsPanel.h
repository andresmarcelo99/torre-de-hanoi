#pragma once
#include "raylib.h"
#include "core/HanoiSolver.h"
#include "app/CallStack.h"

// Panel derecho: las llamadas a solveRec en el movimiento 'current', como pila o como árbol.
// view: 0 = pila, 1 = árbol. El panel lo cambia cuando se hace click en sus pestañas.
Rectangle callsPanelRect(int view, int diskCount);
void drawCallsPanel(int& view, const HanoiSolver& solver, int diskCount, int current);
