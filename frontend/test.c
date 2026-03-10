#include "raylib.h"

int main() {
    InitWindow(800, 450, "Raylib test");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Raylib works!", 190, 200, 20, BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
