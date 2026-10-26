#include "ui/UiCommon.h"

Font uiFont;

void drawText(const char* text, float x, float y, float size, Color color) {
    DrawTextEx(uiFont, text, {x, y}, size, 0.0f, color);
}

Color diskColor(int disk, int diskCount) {
    return ColorFromHSV(300.0f * (disk - 1) / diskCount, 0.6f, 0.9f);
}

char pegName(int peg) { return 'A' + peg; }
