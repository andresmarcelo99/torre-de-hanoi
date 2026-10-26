#include <cmath>
#include "raylib.h"

// raygui es una librería de un solo header: RAYGUI_IMPLEMENTATION incluye su código
// (no solo las declaraciones). Debe definirse en UN solo .cpp del proyecto.
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "core/HanoiSolver.h"
#include "app/TowerState.h"
#include "app/CallsPanel.h"
#include "app/UiCommon.h"

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

    // Fuente de la interfaz (la de raylib por defecto es pixelada y cuesta leerla).
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
        bool mouseOverUi = CheckCollisionPointRec(mouse, PANEL) || CheckCollisionPointRec(mouse, callsPanelRect(app.callsView, app.diskCount));
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
        drawCallsPanel(app.callsView, app.solver, app.diskCount, app.nextMove);
        EndDrawing();
    }

    UnloadFont(uiFont);
    CloseWindow();
    return 0;
}
