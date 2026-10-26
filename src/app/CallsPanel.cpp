#include "app/CallsPanel.h"
#include <cmath>
#include "raygui.h"   // solo declaraciones: la implementación está en main.cpp
#include "app/UiCommon.h"

// 'static' en las funciones de abajo = solo se usan dentro de este archivo

// ---------------------------------------------------------------------------
// Panel de llamadas a solveRec: como pila o como árbol
// ---------------------------------------------------------------------------
constexpr float STACK_W = 370.0f;
constexpr float TREE_W = 460.0f;
constexpr float CARD_H = 54.0f;   // alto de cada caja de la pila
constexpr float TREE_ROW_H = 40.0f;

// El panel crece con la cantidad de discos y se pega al borde derecho de la ventana
Rectangle callsPanelRect(int view, int diskCount) {
    // Como máximo hay diskCount llamadas en la pila (o niveles en el árbol)
    if (view == 0) return {GetScreenWidth() - STACK_W - 10, 10, STACK_W, 112 + diskCount * CARD_H + 40};
    return {GetScreenWidth() - TREE_W - 10, 10, TREE_W, 76 + diskCount * TREE_ROW_H + 86};
}

static const char* discos(int n) { return TextFormat(n == 1 ? "%d disco" : "%d discos", n); }

// Uno de los 3 pasos de solveRec dentro de una caja de la pila.
// state: -1 = ya hecho, 0 = haciéndose ahora, 1 = todavía no
static void drawStep(float x, float y, float w, const char* text, int state, bool empty) {
    Color fg = state < 0 ? DARKGREEN : state == 0 ? BLACK : GRAY;
    if (state == 0) DrawRectangleRec({x - 3, y - 2, w - 2, 18}, Fade(GOLD, 0.55f));
    if (empty) fg = Fade(fg, 0.45f);   // con n = 1 los pasos 1 y 3 no hacen nada
    drawText(text, x, y, 12, fg);
}

// La pila se dibuja como una pila de verdad: la primera llamada abajo, la más nueva arriba.
// Cada caja es una llamada que todavía no terminó; las de abajo esperan a que vuelva la de arriba.
static void drawCallStack(int diskCount, bool finished, int current, Rectangle panel) {
    float x = panel.x + 12;
    float w = panel.width - 24;
    float y = panel.y + 70;

    drawText("Cada caja es una llamada que todavia no termino.", x, y, 13, DARKGRAY);
    drawText("La de arriba se ejecuta; las de abajo la esperan.", x, y + 16, 13, DARKGRAY);
    y += 42;

    if (finished) {
        drawText("Pila vacia: todas las llamadas terminaron", x, y, 16, DARKGREEN);
        return;
    }

    Frame frames[MAX_DISKS];
    int depth = callStackAt(diskCount, current, frames);
    for (int level = depth - 1; level >= 0; level--) {
        const Frame& f = frames[level];
        bool top = (level == depth - 1);
        Rectangle card = {x, y, w, CARD_H - 6};

        DrawRectangleRec(card, top ? Fade(diskColor(f.n, diskCount), 0.35f) : WHITE);
        DrawRectangleLinesEx(card, top ? 2 : 1, top ? BLACK : Fade(GRAY, 0.6f));

        // Qué tiene que hacer esta llamada, en palabras, y la llamada tal cual se escribe
        drawText(TextFormat("Mover %s de %c a %c", discos(f.n), pegName(f.from), pegName(f.to)),
                 x + 8, y + 5, 16, BLACK);
        const char* call = TextFormat("solveRec(%d, %c, %c, %c)", f.n, pegName(f.from), pegName(f.to), pegName(f.aux));
        float callW = MeasureTextEx(uiFont, call, 12, 0).x;
        drawText(call, x + w - callW - 8, y + 8, 12, DARKGRAY);

        // Los 3 pasos del algoritmo. phase: 1 = en el paso 1, 0 = en el 2, 2 = en el 3
        int step = f.phase == 1 ? 1 : f.phase == 0 ? 2 : 3;
        float colW = (w - 16) / 3;
        float sy = y + 27;
        bool noSub = (f.n == 1);
        drawStep(x + 8, sy, colW,
                 noSub ? "1. (nada)" : TextFormat("1. %s %c>%c", discos(f.n - 1), pegName(f.from), pegName(f.aux)),
                 step > 1 ? -1 : 0, noSub);
        drawStep(x + 8 + colW, sy, colW,
                 TextFormat("2. disco %d %c>%c", f.n, pegName(f.from), pegName(f.to)),
                 step > 2 ? -1 : step == 2 ? 0 : 1, false);
        drawStep(x + 8 + 2 * colW, sy, colW,
                 noSub ? "3. (nada)" : TextFormat("3. %s %c>%c", discos(f.n - 1), pegName(f.aux), pegName(f.to)),
                 step == 3 ? 0 : 1, noSub);

        y += CARD_H;
    }

    drawText(TextFormat("abajo de todo: solve(%d), que hizo la primera llamada", diskCount), x, y, 12, GRAY);
    drawText("Verde: hecho   Amarillo: haciendose ahora   Gris: falta", x, y + 18, 12, GRAY);
}

// Árbol de llamadas. El nodo k es la llamada que hace el movimiento k (ver callStatus en CallStack.cpp):
// va en la columna k, en la fila de su n (raíz arriba).
static void drawCallTree(const HanoiSolver& solver, int diskCount, bool finished, int current, Rectangle panel) {
    int total = solver.count();
    int N = diskCount;
    float left = panel.x + 44;                 // espacio a la izquierda para las etiquetas "n=3"
    float top = panel.y + 84;
    float colW = (panel.x + panel.width - 10 - left) / total;
    float radius = fminf(14.0f, colW / 2 - 1);
    if (radius < 2) radius = 2;

    auto nodePos = [&](int k) {
        int n = solver.move(k).disk;
        return Vector2{left + (k + 0.5f) * colW, top + (N - n) * TREE_ROW_H};
    };
    // -1 = ya retornó, 0 = en la pila, 1 = todavía no se llamó
    auto status = [&](int k) { return callStatus(k, solver.move(k).disk, current); };

    for (int n = N; n >= 1; n--) {
        drawText(TextFormat("n=%d", n), panel.x + 10, top + (N - n) * TREE_ROW_H - 8, 13, GRAY);
    }

    // Primero las aristas, para que los nodos queden encima. Los hijos del nodo k
    // (disco n) están a una distancia 2^(n-2) a cada lado.
    for (int k = 0; k < total; k++) {
        int n = solver.move(k).disk;
        if (n == 1) continue;
        int d = 1 << (n - 2);
        int children[2] = {k - d, k + d};
        for (int child : children) {
            bool onPath = status(child) == 0;
            DrawLineEx(nodePos(k), nodePos(child), onPath ? 3.0f : 1.0f,
                       onPath ? BLACK : Fade(GRAY, status(child) < 0 ? 0.8f : 0.35f));
        }
    }

    int hovered = -1;
    for (int k = 0; k < total; k++) {
        const Move& m = solver.move(k);
        Vector2 p = nodePos(k);
        Color c = diskColor(m.disk, N);
        int st = status(k);

        if (st < 0) {
            DrawCircleV(p, radius, Fade(c, 0.45f));                 // ya retornó
        } else if (st == 0) {
            DrawCircleV(p, radius, c);                              // en la pila
            DrawCircleLinesV(p, radius, BLACK);
        } else {
            DrawCircleV(p, radius, WHITE);                          // todavía no se llamó
            DrawCircleLinesV(p, radius, Fade(GRAY, 0.6f));
        }
        if (k == current) {
            DrawRing(p, radius + 2, radius + 5, 0, 360, 32, BLACK);  // la llamada que mueve ahora
        }
        if (radius >= 8) {
            const char* num = TextFormat("%d", m.disk);
            Vector2 sz = MeasureTextEx(uiFont, num, 13, 0);
            drawText(num, p.x - sz.x / 2, p.y - sz.y / 2, 13, st > 0 ? GRAY : BLACK);
        }
        if (colW >= 30) {
            const char* arrow = TextFormat("%c>%c", pegName(m.from), pegName(m.to));
            Vector2 sz = MeasureTextEx(uiFont, arrow, 12, 0);
            drawText(arrow, p.x - sz.x / 2, p.y + radius + 2, 12, st > 0 ? LIGHTGRAY : DARKGRAY);
        }
        if (CheckCollisionPointCircle(GetMousePosition(), p, fmaxf(radius, colW / 2))) hovered = k;
    }

    // Abajo: la llamada bajo el mouse, o si no, la actual
    float y = top + (N - 1) * TREE_ROW_H + 44;
    int shown = hovered >= 0 ? hovered : (finished ? -1 : current);
    if (shown >= 0) {
        const Move& m = solver.move(shown);
        int aux = 3 - m.from - m.to;
        drawText(TextFormat("%s solveRec(%d, %c, %c, %c)", hovered >= 0 ? "Mouse:" : "Ahora:",
                            m.disk, pegName(m.from), pegName(m.to), pegName(aux)),
                 panel.x + 12, y, 16, BLACK);
        drawText(TextFormat("movimiento %d de %d", shown + 1, total), panel.x + 12, y + 22, 13, DARKGRAY);
    } else {
        drawText("Terminado: todas las llamadas retornaron", panel.x + 12, y, 16, DARKGREEN);
    }
    drawText("Relleno: en la pila   Claro: retorno   Blanco: pendiente", panel.x + 12, y + 42, 12, GRAY);
}

void drawCallsPanel(int& view, const HanoiSolver& solver, int diskCount, int current) {
    Rectangle panel = callsPanelRect(view, diskCount);
    bool finished = current >= solver.count();
    GuiPanel(panel, "Llamadas a solveRec");
    GuiToggleGroup({panel.x + 12, panel.y + 34, (panel.width - 24) / 2, 26}, "Pila;Arbol", &view);
    if (view == 0) drawCallStack(diskCount, finished, current, panel);
    else drawCallTree(solver, diskCount, finished, current, panel);
}
