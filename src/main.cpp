#include <cmath>
#include "raylib.h"

// raygui es una librería de un solo header: RAYGUI_IMPLEMENTATION incluye su código
// (no solo las declaraciones). Debe definirse en UN solo .cpp del proyecto.
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "core/HanoiSolver.h"
#include "core/TowerState.h"

// ---------------------------------------------------------------------------
// Medidas de la escena 3D (en unidades del mundo)
// ---------------------------------------------------------------------------
constexpr float PEG_SPACING = 4.0f;    // distancia entre torres
constexpr float BASE_HEIGHT = 0.4f;
constexpr float DISK_HEIGHT = 0.35f;
constexpr float MIN_RADIUS  = 0.45f;   // radio del disco más pequeño
constexpr float MAX_RADIUS  = 1.8f;    // radio del disco más grande
constexpr float PEG_RADIUS  = 0.12f;
constexpr float PEG_HEIGHT  = MAX_DISKS * DISK_HEIGHT + 0.8f;
constexpr float LIFT_Y      = BASE_HEIGHT + PEG_HEIGHT + 0.6f;   // altura a la que se levanta un disco

constexpr float MOVE_SECONDS = 1.0f;   // duración de un movimiento a velocidad 1x

// Posición x de la torre 0, 1, 2  ->  -4, 0, 4
float pegX(int peg) { return (peg - 1) * PEG_SPACING; }

// Altura de la base de un disco que está en el nivel 'level' de una torre (0 = abajo)
float levelY(int level) { return BASE_HEIGHT + level * DISK_HEIGHT; }

// El radio crece de forma lineal con el número de disco
float diskRadius(int disk, int diskCount) {
    if (diskCount <= 1) return MAX_RADIUS;
    return MIN_RADIUS + (disk - 1) * (MAX_RADIUS - MIN_RADIUS) / (diskCount - 1);
}

// Cada disco toma un color distinto del círculo cromático (arcoíris)
Color diskColor(int disk, int diskCount) {
    return ColorFromHSV(300.0f * (disk - 1) / diskCount, 0.6f, 0.9f);
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Suaviza el movimiento: empieza y termina despacio en vez de a velocidad constante
float ease(float t) { return t * t * (3.0f - 2.0f * t); }

// ---------------------------------------------------------------------------
// Estado de la aplicación
// ---------------------------------------------------------------------------
struct App {
    int diskCount = 3;
    HanoiSolver solver;
    TowerState state{3};

    int nextMove = 0;        // índice en solver del próximo movimiento a animar
    bool playing = false;    // reproducción automática activada
    bool animating = false;  // hay un disco en el aire ahora mismo
    float t = 0.0f;          // progreso del movimiento actual, de 0 a 1
    float speed = 1.0f;
    int callsView = 1;       // panel derecho: 0 = pila de llamadas, 1 = árbol de llamadas
};

// Vuelve al inicio: recalcula la solución y pone todos los discos en la torre 0
void reset(App& app) {
    app.solver.solve(app.diskCount);
    app.state = TowerState(app.diskCount);
    app.nextMove = 0;
    app.playing = false;
    app.animating = false;
    app.t = 0.0f;
}

bool finished(const App& app) { return app.nextMove >= app.solver.count(); }

// Empieza a animar el siguiente movimiento (si queda alguno)
void startMove(App& app) {
    if (app.animating || finished(app)) return;
    app.animating = true;
    app.t = 0.0f;
}

// Avanza la animación. dt = segundos desde el frame anterior, así la velocidad
// es la misma con 30 o con 144 FPS.
void update(App& app, float dt) {
    if (app.playing && !app.animating) {
        startMove(app);
    }
    if (!app.animating) return;

    app.t += dt * app.speed / MOVE_SECONDS;
    if (app.t >= 1.0f) {
        // El disco llegó: recién ahora se aplica el movimiento al tablero.
        // Mientras vuela, TowerState todavía lo tiene en la torre de origen.
        app.state.apply(app.solver.move(app.nextMove));
        app.nextMove++;
        app.animating = false;
        if (finished(app)) app.playing = false;
    }
}

// Posición del disco en el aire. El movimiento tiene 3 fases de igual duración:
//   t en [0, 1/3)   subir desde la torre origen
//   t en [1/3, 2/3) deslizar en arco hacia la torre destino
//   t en [2/3, 1]   bajar sobre la torre destino
Vector3 movingDiskPosition(const App& app) {
    const Move& m = app.solver.move(app.nextMove);
    float startX = pegX(m.from);
    float startY = levelY(app.state.size(m.from) - 1);   // está arriba de todo en el origen
    float endX = pegX(m.to);
    float endY = levelY(app.state.size(m.to));           // queda encima del último disco del destino

    const float third = 1.0f / 3.0f;
    if (app.t < third) {
        float k = ease(app.t / third);
        return {startX, lerp(startY, LIFT_Y, k), 0.0f};
    }
    if (app.t < 2 * third) {
        float k = ease((app.t - third) / third);
        float arc = sinf(k * PI) * 0.6f;                  // pequeña curva hacia arriba
        return {lerp(startX, endX, k), LIFT_Y + arc, 0.0f};
    }
    float k = ease((app.t - 2 * third) / third);
    return {endX, lerp(LIFT_Y, endY, k), 0.0f};
}

// ---------------------------------------------------------------------------
// Dibujo 3D
// ---------------------------------------------------------------------------
// DrawCylinder recibe la posición de la BASE del cilindro, no del centro
void drawDisk(Vector3 bottom, int disk, int diskCount) {
    float r = diskRadius(disk, diskCount);
    DrawCylinder(bottom, r, r, DISK_HEIGHT, 40, diskColor(disk, diskCount));
    DrawCylinderWires(bottom, r, r, DISK_HEIGHT, 40, Fade(BLACK, 0.25f));
}

void drawScene(const App& app) {
    // Base de madera
    Vector3 baseCenter = {0.0f, BASE_HEIGHT / 2, 0.0f};
    float baseWidth = 3 * PEG_SPACING + 1.5f;
    DrawCube(baseCenter, baseWidth, BASE_HEIGHT, 4.5f, {139, 94, 60, 255});
    DrawCubeWires(baseCenter, baseWidth, BASE_HEIGHT, 4.5f, {90, 60, 38, 255});

    // Torres
    for (int p = 0; p < 3; p++) {
        DrawCylinder({pegX(p), BASE_HEIGHT, 0.0f}, PEG_RADIUS, PEG_RADIUS, PEG_HEIGHT, 16, {200, 170, 130, 255});
    }

    // Discos apoyados en las torres. Si un disco está en el aire, TowerState aún lo
    // tiene arriba de su torre de origen: ese lo saltamos y lo dibujamos aparte.
    int flyingPeg = app.animating ? app.solver.move(app.nextMove).from : -1;
    for (int p = 0; p < 3; p++) {
        for (int level = 0; level < app.state.size(p); level++) {
            bool isFlying = (p == flyingPeg && level == app.state.size(p) - 1);
            if (isFlying) continue;
            drawDisk({pegX(p), levelY(level), 0.0f}, app.state.diskAt(p, level), app.diskCount);
        }
    }

    if (app.animating) {
        drawDisk(movingDiskPosition(app), app.solver.move(app.nextMove).disk, app.diskCount);
    }
}

// ---------------------------------------------------------------------------
// Cámara orbital: arrastrar con el mouse para rotar, rueda para zoom
// ---------------------------------------------------------------------------
struct Orbit {
    float yaw = 0.0f;        // ángulo horizontal
    float pitch = 0.45f;     // ángulo vertical
    float distance = 17.0f;
    bool dragging = false;
};

void updateCamera(Orbit& orbit, Camera3D& cam, bool mouseOverUi) {
    // Solo rota si el arrastre empezó fuera del panel (para no rotar al usar botones)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) orbit.dragging = !mouseOverUi;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) orbit.dragging = false;

    if (orbit.dragging) {
        Vector2 delta = GetMouseDelta();
        orbit.yaw -= delta.x * 0.006f;
        orbit.pitch += delta.y * 0.006f;
        if (orbit.pitch < 0.05f) orbit.pitch = 0.05f;
        if (orbit.pitch > 1.45f) orbit.pitch = 1.45f;
    }
    if (!mouseOverUi) {
        orbit.distance -= GetMouseWheelMove() * 1.0f;
        if (orbit.distance < 7.0f) orbit.distance = 7.0f;
        if (orbit.distance > 35.0f) orbit.distance = 35.0f;
    }

    // Coordenadas esféricas -> posición de la cámara alrededor del objetivo
    cam.position = {
        cam.target.x + orbit.distance * cosf(orbit.pitch) * sinf(orbit.yaw),
        cam.target.y + orbit.distance * sinf(orbit.pitch),
        cam.target.z + orbit.distance * cosf(orbit.pitch) * cosf(orbit.yaw),
    };
}

// ---------------------------------------------------------------------------
// Interfaz (raygui): botones, slider y textos
// ---------------------------------------------------------------------------
const Rectangle PANEL = {10, 10, 300, 330};

// Fuente de la interfaz. La de raylib por defecto es pixelada y cuesta leerla.
Font uiFont;

// Igual que DrawText, pero con nuestra fuente
void drawText(const char* text, float x, float y, float size, Color color) {
    DrawTextEx(uiFont, text, {x, y}, size, 0.0f, color);
}

void drawUi(App& app) {
    GuiPanel(PANEL, "Torre de Hanoi");
    float x = PANEL.x + 12;
    float y = PANEL.y + 40;

    // Cantidad de discos: - [n] +
    drawText("Discos", x, y + 6, 16, DARKGRAY);
    if (GuiButton({x + 110, y, 30, 28}, "-") && app.diskCount > 1) {
        app.diskCount--;
        reset(app);
    }
    drawText(TextFormat("%d", app.diskCount), x + 155, y + 6, 16, BLACK);
    if (GuiButton({x + 185, y, 30, 28}, "+") && app.diskCount < MAX_DISKS) {
        app.diskCount++;
        reset(app);
    }
    y += 40;

    // Reproducir / pausar, paso a paso, reiniciar
    if (GuiButton({x, y, 90, 30}, app.playing ? "Pausa" : "Play")) {
        if (finished(app)) reset(app);
        app.playing = !app.playing;
    }
    if (GuiButton({x + 95, y, 90, 30}, "Paso")) {
        app.playing = false;
        startMove(app);
    }
    if (GuiButton({x + 190, y, 86, 30}, "Reiniciar")) {
        reset(app);
    }
    y += 42;

    // Velocidad
    drawText("Velocidad", x, y + 4, 16, DARKGRAY);
    GuiSliderBar({x + 95, y, 130, 22}, nullptr, TextFormat("%.1fx", app.speed), &app.speed, 0.25f, 10.0f);
    y += 40;

    // Contadores
    int total = app.solver.count();
    drawText(TextFormat("Movimiento %d / %d", app.nextMove, total), x, y, 18, BLACK);
    y += 28;
    drawText(TextFormat("Total: 2^%d - 1 = %d", app.diskCount, total), x, y, 16, DARKGRAY);
    y += 34;

    // Movimiento actual o siguiente
    if (finished(app)) {
        drawText("Resuelto!", x, y, 20, DARKGREEN);
    } else {
        const Move& m = app.solver.move(app.nextMove);
        const char* label = app.animating ? "Moviendo" : "Siguiente";
        drawText(TextFormat("%s: disco %d", label, m.disk), x, y, 18, BLACK);
        drawText(TextFormat("%c -> %c", 'A' + m.from, 'A' + m.to), x, y + 24, 18, BLACK);
    }

    drawText("Espacio: play/pausa  Der: paso  R: reiniciar", x, PANEL.y + PANEL.height - 46, 13, GRAY);
    drawText("Arrastrar: rotar  Rueda: zoom  T: pila/arbol", x, PANEL.y + PANEL.height - 28, 13, GRAY);
}

// ---------------------------------------------------------------------------
// Panel de llamadas a solveRec: como pila o como árbol
// ---------------------------------------------------------------------------
constexpr float STACK_W = 370.0f;
constexpr float TREE_W = 460.0f;
constexpr float CARD_H = 54.0f;   // alto de cada caja de la pila
constexpr float TREE_ROW_H = 40.0f;

// El panel crece con la cantidad de discos y se pega al borde derecho de la ventana
Rectangle callsPanel(const App& app) {
    // Como máximo hay diskCount llamadas en la pila (o niveles en el árbol)
    if (app.callsView == 0) return {GetScreenWidth() - STACK_W - 10, 10, STACK_W, 112 + app.diskCount * CARD_H + 40};
    return {GetScreenWidth() - TREE_W - 10, 10, TREE_W, 76 + app.diskCount * TREE_ROW_H + 86};
}

char pegName(int peg) { return 'A' + peg; }

const char* discos(int n) { return TextFormat(n == 1 ? "%d disco" : "%d discos", n); }

// Uno de los 3 pasos de solveRec dentro de una caja de la pila.
// state: -1 = ya hecho, 0 = haciéndose ahora, 1 = todavía no
void drawStep(float x, float y, float w, const char* text, int state, bool empty) {
    Color fg = state < 0 ? DARKGREEN : state == 0 ? BLACK : GRAY;
    if (state == 0) DrawRectangleRec({x - 3, y - 2, w - 2, 18}, Fade(GOLD, 0.55f));
    if (empty) fg = Fade(fg, 0.45f);   // con n = 1 los pasos 1 y 3 no hacen nada
    drawText(text, x, y, 12, fg);
}

// La pila se dibuja como una pila de verdad: la primera llamada abajo, la más nueva arriba.
// Cada caja es una llamada que todavía no terminó; las de abajo esperan a que vuelva la de arriba.
void drawCallStack(const App& app, Rectangle panel) {
    float x = panel.x + 12;
    float w = panel.width - 24;
    float y = panel.y + 70;

    drawText("Cada caja es una llamada que todavia no termino.", x, y, 13, DARKGRAY);
    drawText("La de arriba se ejecuta; las de abajo la esperan.", x, y + 16, 13, DARKGRAY);
    y += 42;

    if (finished(app)) {
        drawText("Pila vacia: todas las llamadas terminaron", x, y, 16, DARKGREEN);
        return;
    }

    int i = app.nextMove;
    int depth = app.solver.depth(i);
    for (int level = depth - 1; level >= 0; level--) {
        const Frame& f = app.solver.frame(i, level);
        bool top = (level == depth - 1);
        Rectangle card = {x, y, w, CARD_H - 6};

        DrawRectangleRec(card, top ? Fade(diskColor(f.n, app.diskCount), 0.35f) : WHITE);
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

    drawText(TextFormat("abajo de todo: solve(%d), que hizo la primera llamada", app.diskCount), x, y, 12, GRAY);
    drawText("Verde: hecho   Amarillo: haciendose ahora   Gris: falta", x, y + 18, 12, GRAY);
}

// Árbol de llamadas. Truco: cada llamada con n > 0 hace exactamente un movimiento, y si
// el árbol se recorre en orden (hijo izq, nodo, hijo der) los nodos salen en el mismo orden
// que los movimientos. Así que el nodo k es el movimiento k: va en la columna k, en la fila
// de su n (raíz arriba). Su sub-árbol ocupa las columnas k - h .. k + h, con h = 2^(n-1) - 1.
void drawCallTree(const App& app, Rectangle panel) {
    int total = app.solver.count();
    int N = app.diskCount;
    float left = panel.x + 44;                 // espacio a la izquierda para las etiquetas "n=3"
    float top = panel.y + 84;
    float colW = (panel.x + panel.width - 10 - left) / total;
    float radius = fminf(14.0f, colW / 2 - 1);
    if (radius < 2) radius = 2;

    auto nodePos = [&](int k) {
        int n = app.solver.move(k).disk;
        return Vector2{left + (k + 0.5f) * colW, top + (N - n) * TREE_ROW_H};
    };
    // -1 = ya retornó, 0 = en la pila, 1 = todavía no se llamó
    auto status = [&](int k) {
        int h = (1 << (app.solver.move(k).disk - 1)) - 1;
        if (k + h < app.nextMove) return -1;
        if (k - h > app.nextMove) return 1;
        return 0;
    };

    for (int n = N; n >= 1; n--) {
        drawText(TextFormat("n=%d", n), panel.x + 10, top + (N - n) * TREE_ROW_H - 8, 13, GRAY);
    }

    // Primero las aristas, para que los nodos queden encima. Los hijos del nodo k
    // (disco n) están a una distancia 2^(n-2) a cada lado.
    for (int k = 0; k < total; k++) {
        int n = app.solver.move(k).disk;
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
        const Move& m = app.solver.move(k);
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
        if (k == app.nextMove) {
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
    int shown = hovered >= 0 ? hovered : (finished(app) ? -1 : app.nextMove);
    if (shown >= 0) {
        const Move& m = app.solver.move(shown);
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

void drawCallsPanel(App& app) {
    Rectangle panel = callsPanel(app);
    GuiPanel(panel, "Llamadas a solveRec");
    GuiToggleGroup({panel.x + 12, panel.y + 34, (panel.width - 24) / 2, 26}, "Pila;Arbol", &app.callsView);
    if (app.callsView == 0) drawCallStack(app, panel);
    else drawCallTree(app, panel);
}

// Atajos de teclado
void handleKeys(App& app) {
    if (IsKeyPressed(KEY_SPACE)) {
        if (finished(app)) reset(app);
        app.playing = !app.playing;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        app.playing = false;
        startMove(app);
    }
    if (IsKeyPressed(KEY_R)) reset(app);
    if (IsKeyPressed(KEY_T)) app.callsView = 1 - app.callsView;
}

// ---------------------------------------------------------------------------
// Programa principal: el game loop
// ---------------------------------------------------------------------------
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);   // bordes suaves, ventana redimensionable
    InitWindow(1280, 720, "Torre de Hanoi");
    SetTargetFPS(60);

    // Se carga grande (32 px) y se dibuja más chica con filtro bilineal para que se vea nítida.
    // GetApplicationDirectory() hace que funcione aunque el programa se lance desde otra carpeta.
    uiFont = LoadFontEx(TextFormat("%sassets/fonts/Inter-Medium.ttf", GetApplicationDirectory()), 32, nullptr, 0);
    SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(uiFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 0);

    Camera3D cam{};
    cam.target = {0.0f, 1.5f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    Orbit orbit;

    App app;
    reset(app);

    while (!WindowShouldClose()) {
        // 1. Entrada y lógica
        Vector2 mouse = GetMousePosition();
        bool mouseOverUi = CheckCollisionPointRec(mouse, PANEL) || CheckCollisionPointRec(mouse, callsPanel(app));
        handleKeys(app);
        updateCamera(orbit, cam, mouseOverUi);
        update(app, GetFrameTime());

        // 2. Dibujo: primero el mundo 3D, después la interfaz 2D encima
        BeginDrawing();
        ClearBackground({235, 238, 243, 255});

        BeginMode3D(cam);
        drawScene(app);
        EndMode3D();

        drawUi(app);
        drawCallsPanel(app);
        EndDrawing();
    }

    UnloadFont(uiFont);
    CloseWindow();
    return 0;
}
