#pragma once
#include "raylib.h"

// Cosas de dibujo que comparten main.cpp y los paneles
extern Font uiFont;   // fuente de la interfaz; se carga en main()

// Igual que DrawText, pero con nuestra fuente
void drawText(const char* text, float x, float y, float size, Color color);

// Cada disco toma un color distinto del círculo cromático (arcoíris)
Color diskColor(int disk, int diskCount);

// Torre 0, 1, 2  ->  'A', 'B', 'C'
char pegName(int peg);
