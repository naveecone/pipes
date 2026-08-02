#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "pipes.h"

static const int PIPE_BASE_MASK[4] = {
    [PIPE_TWO] = DIR_WEST | DIR_EAST,
    [PIPE_THREE] = DIR_NORTH | DIR_EAST | DIR_WEST,
    [PIPE_FOUR] = DIR_NORTH | DIR_EAST | DIR_WEST | DIR_SOUTH,
    [PIPE_CORNER] = DIR_NORTH | DIR_EAST
};

void init_world(World *world) {
    world->entities_data.cap = INIT_ARR_SIZE;
    world->entities = malloc(sizeof(Entity) * INIT_ARR_SIZE);
    assert(world->entities != NULL);
    world->entities_data.size = 0;

    for (int x = 0; x < WORLD_SIZE; ++x)
        for (int y = 0; y < WORLD_SIZE; ++y)
            world->tiles[x][y].entity_idx = -1;
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

void add_entity(World *world, Entity e, EntityType type) {
    if (world->tiles[e.pos.x][e.pos.y].entity_idx != -1) {
        printf("Something already exists at (%d, %d)\n", e.pos.x, e.pos.y);
        return;
    }

    ArrData *entities_data = &world->entities_data;

    if (entities_data->size >= entities_data->cap) {
        entities_data->cap *= 2;
        world->entities = realloc(world->entities, sizeof(Entity) * entities_data->cap);
        assert(world->entities != NULL);
    }

    Entity *cur = &world->entities[entities_data->size];
    *cur = e;

    world->tiles[e.pos.x][e.pos.y].entity_idx = entities_data->size;

    ++entities_data->size;
    printf("Added entity at (%d, %d)\n", e.pos.x, e.pos.y);

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
    *target = from + ((*target - from + 1) % (to - from));
}

void process_input(State *state, Vector2 mouse_pos) {
    Vector2i tile_pos = get_tile_pos(mouse_pos);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Entity e = { 0 };

        e.pos = tile_pos;

        bool is_pipe = state->building_type >= ENTITY_PIPE_TWO &&
            state->building_type <= ENTITY_PIPE_CORNER;
        if (is_pipe) {
            e.rotation = state->building_rotation;
            PipeType ptype = state->building_type - ENTITY_PIPE_TWO;
            e.dir = rotate4(PIPE_BASE_MASK[ptype], state->building_rotation);
            e.ptype = ptype;
            e.type = state->building_type;
        } else {
            e.dir = 0;
            e.rotation = 0;
        }

        add_entity(&state->world, e, state->building_type); 
        state->no_place_attempts = false;
    }

    if (IsKeyPressed(KEY_G))
        state->render_grid = !state->render_grid;

    if (IsKeyPressed(KEY_R))
        cycle_range(&state->building_rotation, 0, 4);

    if (IsKeyPressed(KEY_SPACE))
        cycle_range((int *) &state->building_type, 0, ENTITIES_TYPE_COUNT);

    if (IsKeyPressed(KEY_B)) 
        state->building_mode = !state->building_mode;
}

void cleanup(State *state) {
    free(state->world.entities);

    for (size_t i = 0; i < TEXTURES_SIZE; ++i)
        UnloadTexture(state->textures[i]);
}

int main() {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    State state;
    state.textures[ENTITY_WOODEN_BOX]  = LoadTexture("sprites/wooden_box.png");
    state.textures[ENTITY_PIPE_TWO]    = LoadTexture("sprites/pipe_2.png");
    state.textures[ENTITY_PIPE_THREE]  = LoadTexture("sprites/pipe_3.png");
    state.textures[ENTITY_PIPE_FOUR]   = LoadTexture("sprites/pipe_4.png");
    state.textures[ENTITY_PIPE_CORNER] = LoadTexture("sprites/pipe_corner.png");

    state.no_place_attempts = true;
    state.building_mode = true;
    state.building_rotation = 0;
    state.building_type = ENTITY_WOODEN_BOX;
    state.render_grid = false;
    init_world(&state.world);

    Rectangle src = { 0, 0, SPRITE_SIZE, SPRITE_SIZE };
    Vector2 origin = { TILE_SIZE / 2, TILE_SIZE / 2 };

    state.building_type = ENTITY_WOODEN_BOX;
    state.building_rotation = 0;
    state.render_grid = false;

    while (!WindowShouldClose()) {  
        Vector2 mouse_pos = GetMousePosition();
        Vector2i tile_pos = get_tile_pos(mouse_pos);
        process_input(&state, mouse_pos);

        BeginDrawing();
        ClearBackground(BLACK);

        if (state.building_mode) {
            int pos_x = tile_pos.x * TILE_SIZE + TILE_SIZE / 2;
            int pos_y = tile_pos.y * TILE_SIZE + TILE_SIZE / 2;
            Rectangle shadow_entity_dest = { pos_x, pos_y, TILE_SIZE, TILE_SIZE };
            int rotation = state.building_type >= ENTITY_PIPE_TWO && state.building_type <= ENTITY_PIPE_CORNER ? state.building_rotation : 0;
            DrawTexturePro(state.textures[state.building_type], src, shadow_entity_dest, origin, rotation * 90, BLUE);
        }

        for (size_t i = 0; i < state.world.entities_data.size; ++i) {
            Rectangle pipe_dest = { 0, 0, TILE_SIZE, TILE_SIZE };
            pipe_dest.x = state.world.entities[i].pos.x * TILE_SIZE + TILE_SIZE / 2; 
            pipe_dest.y = state.world.entities[i].pos.y * TILE_SIZE + TILE_SIZE / 2; 
            DrawTexturePro(state.textures[state.world.entities[i].type], src, pipe_dest, origin, state.world.entities[i].rotation * 90, WHITE);
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
