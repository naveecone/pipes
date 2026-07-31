#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define WORLD_SIZE 30
#define TILE_SIZE 64
#define INIT_ARR_SIZE 32
#define WINDOW_TITLE "Pipes"
#define WINDOW_WIDTH 1800
#define WINDOW_HEIGHT 900

typedef enum {
    PIPE_TWO,
    PIPE_THREE,
    PIPE_FOUR,
    PIPE_CORNER
} PipeType;

typedef struct {
    int x;
    int y;
} Vector2i;

typedef struct {
    PipeType type;
    Vector2i pos;

    int rotation;
} Pipe;

typedef struct {
    ssize_t pipe_idx;
} Tile;

typedef struct {
    size_t size;
    size_t cap;
} ArrData;

typedef struct {
    Tile tiles[WORLD_SIZE][WORLD_SIZE];
    ArrData tiles_data;

    Pipe *pipes;
    ArrData pipes_data;
} World;

typedef struct {
    World world;
    
    int building_rotation;
    PipeType building_type;

    bool render_grid;
} State;

void init_world(World *world) {
    world->pipes_data.cap = INIT_ARR_SIZE;
    world->pipes = malloc(sizeof(Pipe) * INIT_ARR_SIZE);
    assert(world->pipes != NULL);
    world->pipes_data.size = 0;

    for (int x = 0; x < WORLD_SIZE; ++x)
        for (int y = 0; y < WORLD_SIZE; ++y)
            world->tiles[x][y].pipe_idx = -1;
}

void add_pipe(World *world, Vector2i pos, PipeType type, int rotation) {
    if (world->tiles[pos.x][pos.y].pipe_idx != -1) {
        printf("Pipe already exists at (%d, %d)\n", pos.x, pos.y);
        return;
    }

    ArrData *pipes_data = &world->pipes_data;

    if (pipes_data->size >= pipes_data->cap) {
        pipes_data->cap *= 2;
        world->pipes = realloc(world->pipes, sizeof(Pipe) * pipes_data->cap);
        assert(world->pipes != NULL);
    }

    Pipe *cur = &world->pipes[pipes_data->size];

    world->tiles[pos.x][pos.y].pipe_idx = pipes_data->size;
    cur->type = type;
    cur->rotation = rotation;
    cur->pos = pos;
    ++pipes_data->size;
    printf("Added pipe at (%d, %d)\n", pos.x, pos.y);
}

Vector2i get_tile_pos(Vector2 pos) {
    return (Vector2i) { (int) pos.x / TILE_SIZE, (int) pos.y / TILE_SIZE };
}

void cycle_range(int *target, int from, int to) {

    if (*target < from) *target = from;
    if (*target > to) *target = to;

    *target = (++(*target) + from) % to;
}

int main() {

    bool no_place_attempts = true;
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    Texture pipe_textures[4];
    pipe_textures[PIPE_TWO]    = LoadTexture("sprites/pipe_2.png");
    pipe_textures[PIPE_THREE]  = LoadTexture("sprites/pipe_3.png");
    pipe_textures[PIPE_FOUR]   = LoadTexture("sprites/pipe_4.png");
    pipe_textures[PIPE_CORNER] = LoadTexture("sprites/pipe_corner.png");

    Rectangle src = { 0, 0, 32, 32 };

    State state;
    init_world(&state.world);

    Vector2 origin = { TILE_SIZE / 2, TILE_SIZE / 2 };
    while (!WindowShouldClose()) {  
        BeginDrawing();
        ClearBackground(BLACK);

        Vector2 mouse_pos = (Vector2) GetMousePosition();
        Vector2i tile_pos = get_tile_pos(mouse_pos);

        int pos_x = tile_pos.x * TILE_SIZE + TILE_SIZE / 2;
        int pos_y = tile_pos.y * TILE_SIZE + TILE_SIZE / 2;
        Rectangle shadow_pipe_dest = { pos_x, pos_y, TILE_SIZE, TILE_SIZE };
        DrawTexturePro(pipe_textures[state.building_type], src, shadow_pipe_dest, origin, state.building_rotation * 90, BLUE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            add_pipe(&state.world, tile_pos, state.building_type, state.building_rotation);
            no_place_attempts = false;
        }

        if (IsKeyPressed(KEY_G))
            state.render_grid = !state.render_grid;

        if (IsKeyPressed(KEY_R))
            cycle_range(&state.building_rotation, 0, 4);

        if (IsKeyPressed(KEY_SPACE))
            cycle_range((int *) &state.building_type, 0, 4);

        for (size_t i = 0; i < state.world.pipes_data.size; ++i) {
            Rectangle pipe_dest = { 0, 0, TILE_SIZE, TILE_SIZE };
            pipe_dest.x = state.world.pipes[i].pos.x * TILE_SIZE + TILE_SIZE / 2; 
            pipe_dest.y = state.world.pipes[i].pos.y * TILE_SIZE + TILE_SIZE / 2; 
            DrawTexturePro(pipe_textures[state.world.pipes[i].type], src, pipe_dest, origin, state.world.pipes[i].rotation * 90, WHITE);
        }

        if (state.render_grid) {
            for (int x = 0; x < WORLD_SIZE; ++x)
                for (int y = 0; y < WORLD_SIZE; ++y)
                    DrawRectangleLines(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, WHITE);
        }

        if (no_place_attempts) {
            const char *string = "Press SPACE to cycle pipe types\nPress R to rotate\nPress G to render grid";
            DrawText(string, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, 20, WHITE);
        }
                                                                                                  
        EndDrawing();
    }
    return 0;
}
