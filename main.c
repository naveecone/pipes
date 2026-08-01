#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define WORLD_SIZE 30
#define TILE_SIZE 64
#define SPRITE_SIZE 32
#define INIT_ARR_SIZE 32
#define WINDOW_TITLE "Pipes"
#define WINDOW_WIDTH 1800
#define WINDOW_HEIGHT 900
#define TEXTURES_SIZE 4

typedef enum {
    PIPE_TWO,
    PIPE_THREE,
    PIPE_FOUR,
    PIPE_CORNER
} PipeType;

typedef enum { 
    DIR_NORTH = 1 << 0,
    DIR_EAST  = 1 << 1,
    DIR_SOUTH = 1 << 2,
    DIR_WEST  = 1 << 3,
} Direction;

static const int PIPE_BASE_MASK[4] = {
    [PIPE_TWO] = DIR_WEST | DIR_EAST,
    [PIPE_THREE] = DIR_NORTH | DIR_EAST | DIR_WEST,
    [PIPE_FOUR] = DIR_NORTH | DIR_EAST | DIR_WEST | DIR_SOUTH,
    [PIPE_CORNER] = DIR_NORTH | DIR_EAST
};

typedef struct {
    int x;
    int y;
} Vector2i;

typedef struct {
    PipeType type;
    Vector2i pos;

    Direction dir;
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

    Pipe *pipes;
    ArrData pipes_data;
} World;

typedef struct {
    World world;
    Texture pipe_textures[TEXTURES_SIZE];
    
    int building_rotation;
    PipeType building_type;

    bool render_grid;
    bool building_mode;
    bool no_place_attempts;
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

void print_direction_bits(Direction dir) {
    for (int i = 3; i >= 0; --i) {
        putchar(dir & (1 << i) ? '1' : '0');
    }
    putchar('\n');
}

int rotate4(Direction dir, int n) {
    for (int i = 0; i < n; ++i)
        dir = ((dir << 1) | (dir >> 3)) & 0xF;
    return dir;
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
    cur->dir = rotate4(PIPE_BASE_MASK[type], rotation);
    cur->type = type;
    cur->rotation = rotation;
    cur->pos = pos;
    ++pipes_data->size;
    printf("Added pipe at (%d, %d)\n", pos.x, pos.y);
    printf("Direction mask: ");
    print_direction_bits(cur->dir);
}

Vector2i get_tile_pos(Vector2 pos) {
    int pos_x = pos.x / TILE_SIZE;
    int pos_y = pos.y / TILE_SIZE;

    if (pos_x < 0) pos_x = 0;
    if (pos_y < 0) pos_y = 0;

    if (pos_x >= WORLD_SIZE) pos_x = WORLD_SIZE - 1;
    if (pos_y >= WORLD_SIZE) pos_y = WORLD_SIZE - 1;

    return (Vector2i) { pos_x, pos_y };
}

void cycle_range(int *target, int from, int to) {
    *target = from + ((*target - from + 1) % (to - from + 1));
}

void process_input(State *state, Vector2 mouse_pos) {
    Vector2i tile_pos = get_tile_pos(mouse_pos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        add_pipe(&state->world, tile_pos, state->building_type, state->building_rotation);
        state->no_place_attempts = false;
    }

    if (IsKeyPressed(KEY_G))
        state->render_grid = !state->render_grid;

    if (IsKeyPressed(KEY_R))
        cycle_range(&state->building_rotation, 0, 3);

    if (IsKeyPressed(KEY_SPACE))
        cycle_range((int *) &state->building_type, 0, 3);

    if (IsKeyPressed(KEY_B)) 
        state->building_mode = !state->building_mode;
}

void cleanup(State *state) {
    free(state->world.pipes);

    for (size_t i = 0; i < TEXTURES_SIZE; ++i) {
        UnloadTexture(state->pipe_textures[i]);
    }
}

int main() {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    Rectangle src = { 0, 0, SPRITE_SIZE, SPRITE_SIZE };

    State state;
    state.pipe_textures[PIPE_TWO]    = LoadTexture("sprites/pipe_2.png");
    state.pipe_textures[PIPE_THREE]  = LoadTexture("sprites/pipe_3.png");
    state.pipe_textures[PIPE_FOUR]   = LoadTexture("sprites/pipe_4.png");
    state.pipe_textures[PIPE_CORNER] = LoadTexture("sprites/pipe_corner.png");

    state.no_place_attempts = true;
    state.building_mode = true;
    init_world(&state.world);

    Vector2 origin = { TILE_SIZE / 2, TILE_SIZE / 2 };
    while (!WindowShouldClose()) {  
        Vector2 mouse_pos = (Vector2) GetMousePosition();
        Vector2i tile_pos = get_tile_pos(mouse_pos);
        process_input(&state, mouse_pos);

        BeginDrawing();
        ClearBackground(BLACK);

        if (state.building_mode) {
            int pos_x = tile_pos.x * TILE_SIZE + TILE_SIZE / 2;
            int pos_y = tile_pos.y * TILE_SIZE + TILE_SIZE / 2;
            Rectangle shadow_pipe_dest = { pos_x, pos_y, TILE_SIZE, TILE_SIZE };
            DrawTexturePro(state.pipe_textures[state.building_type], src, shadow_pipe_dest, origin, state.building_rotation * 90, BLUE);
        }

        for (size_t i = 0; i < state.world.pipes_data.size; ++i) {
            Rectangle pipe_dest = { 0, 0, TILE_SIZE, TILE_SIZE };
            pipe_dest.x = state.world.pipes[i].pos.x * TILE_SIZE + TILE_SIZE / 2; 
            pipe_dest.y = state.world.pipes[i].pos.y * TILE_SIZE + TILE_SIZE / 2; 
            DrawTexturePro(state.pipe_textures[state.world.pipes[i].type], src, pipe_dest, origin, state.world.pipes[i].rotation * 90, WHITE);
        }

        if (state.render_grid) {
            for (int x = 0; x < WORLD_SIZE; ++x)
                for (int y = 0; y < WORLD_SIZE; ++y)
                    DrawRectangleLines(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
        }

        if (state.no_place_attempts) {
            const char *string = \
"Press SPACE to change pipe type\n\
Press R to rotate the pipe\n\
Press B to toggle building mode\n\
Press G to render grid\n"; 
            DrawText(string, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, 20, WHITE);
        }
                                                                                                  
        EndDrawing();
    }
    
    cleanup(&state);
    CloseWindow();
    return 0;
}
