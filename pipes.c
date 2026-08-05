#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "pipes.h"

static const Direction directions[] = { DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST };
static const Vector2i sides[] = { 
    {  0, -1 },
    {  1,  0 },
    {  0,  1 },
    { -1,  0 }
};

static const int PIPE_BASE_MASK[4] = {
    [PIPE_TWO] = DIR_WEST | DIR_EAST,
    [PIPE_THREE] = DIR_NORTH | DIR_EAST | DIR_WEST,
    [PIPE_FOUR] = DIR_NORTH | DIR_EAST | DIR_WEST | DIR_SOUTH,
    [PIPE_CORNER] = DIR_NORTH | DIR_EAST
};

static const TextureType entity_texture_map[] = {
    [ENTITY_WOODEN_BOX] = TEX_WOODEN_BOX,
    [ENTITY_PIPE_TWO] = TEX_PIPE_TWO,
    [ENTITY_PIPE_THREE] = TEX_PIPE_THREE,
    [ENTITY_PIPE_FOUR] = TEX_PIPE_FOUR,
    [ENTITY_PIPE_CORNER] = TEX_PIPE_CORNER
};

static const char *entity_to_string_map[] = {
    [ENTITY_WOODEN_BOX] = "Wooden box",
    [ENTITY_PIPE_TWO] = "Direct pipe",
    [ENTITY_PIPE_THREE] = "T-junction",
    [ENTITY_PIPE_FOUR] = "Cross",
    [ENTITY_PIPE_CORNER] = "Elbow"
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

void add_entity(World *world, Entity e) {
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

void render_entities(State *state, Rectangle tex_src, Vector2 tex_origin) {

    for (size_t i = 0; i < state->world.entities_data.size; ++i) {
        Rectangle entity_dest = { 0, 0, TILE_SIZE, TILE_SIZE };
        entity_dest.x = state->world.entities[i].pos.x * TILE_SIZE + TILE_SIZE / 2; 
        entity_dest.y = state->world.entities[i].pos.y * TILE_SIZE + TILE_SIZE / 2; 

        Entity e = state->world.entities[i];
        TextureType tex = entity_texture_map[e.type];
        DrawTexturePro(state->textures[tex], tex_src, entity_dest, tex_origin, state->world.entities[i].rotation * 90, WHITE);

        if (e.type == ENTITY_WOODEN_BOX) {

            if (e.hatch.alive) {
                TextureType t = e.hatch.input ? TEX_INPUT_HATCH : TEX_OUTPUT_HATCH;
                DrawTexturePro(state->textures[t], tex_src, entity_dest, tex_origin, e.hatch.rotation * 90, WHITE);
            }
        }
    }
}

bool is_pipe(EntityType type) {
    return type >= ENTITY_PIPE_TWO && type <= ENTITY_PIPE_CORNER;
}

void render_placement(State *state, Rectangle tex_src, Vector2 tex_origin, Vector2 mouse_pos) {

        Vector2i tile_pos = get_tile_pos(mouse_pos);
        int pos_x = tile_pos.x * TILE_SIZE + TILE_SIZE / 2;
        int pos_y = tile_pos.y * TILE_SIZE + TILE_SIZE / 2;

        Rectangle placement_dest = { pos_x, pos_y, TILE_SIZE, TILE_SIZE };

        if (state->mode == MODE_BUILDING) {
           
            EntityType type = state->building_type;
            bool over_same_type_entity = false;
            int idx = state->world.tiles[tile_pos.x][tile_pos.y].entity_idx;
            if (idx != -1) {
                Entity hover_e = state->world.entities[idx];
                over_same_type_entity = hover_e.type == type;
            }

            int rotation = is_pipe(type) ? state->cursor_rotation : 0;

            if (!over_same_type_entity) {
                TextureType tex = entity_texture_map[state->building_type];
                DrawTexturePro(state->textures[tex], tex_src, placement_dest, tex_origin, rotation * 90, BLUE);
            }
        } else if (state->mode == MODE_HATCH_MOUNTING) {
            
            TextureType t = state->hatch_input ? TEX_INPUT_HATCH : TEX_OUTPUT_HATCH;
            DrawTexturePro(state->textures[t], tex_src, placement_dest, tex_origin, state->cursor_rotation * 90, BLUE);
        }

}

void print_entity_info(Entity e) {
    printf("Position: (%d, %d)\n", e.pos.x, e.pos.y);
    printf("Entity type: %s\n", entity_to_string_map[e.type]);
    printf("Direction: ");
    print_direction_bits(e.dir);
    printf("Rotation: %d\n", e.rotation);
    printf("Has hatch: %s\n", e.hatch.alive ? "true" : "false");
    if (e.hatch.alive) {
        printf("Input: %s\n", e.hatch.input ? "true" : "false");
        int r = e.hatch.rotation;
        printf("Side: (%d, %d)\n", sides[r].x, sides[r].y);
    }
    printf("Items contained: %d\n", e.items_contained);
}

void process_input(State *state, Vector2 mouse_pos) {
    Vector2i tile_pos = get_tile_pos(mouse_pos);
    int cursor_entity_idx = state->world.tiles[tile_pos.x][tile_pos.y].entity_idx;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (state->mode == MODE_INFO) {
            if (cursor_entity_idx != -1) {
                Entity e = state->world.entities[cursor_entity_idx];
                print_entity_info(e);
            }
        } else if (state->mode == MODE_BUILDING) {
            Entity e = { 0 };

            e.pos = tile_pos;
            e.type = state->building_type;
            e.dir = 0;
            e.rotation = 0;

            if (is_pipe(state->building_type)) {
                e.rotation = state->cursor_rotation;
                PipeType ptype = state->building_type - ENTITY_PIPE_TWO;
                e.dir = rotate4(PIPE_BASE_MASK[ptype], state->cursor_rotation);
                e.ptype = ptype;
            } else if (state->building_type == ENTITY_WOODEN_BOX) {
                e.items_contained = 2;
            }

            add_entity(&state->world, e); 
            state->no_place_attempts = false;
        } else if (state->mode == MODE_HATCH_MOUNTING) {
            if (cursor_entity_idx != -1) {
                Entity *cursor_e = &state->world.entities[cursor_entity_idx];

                if (cursor_e->type == ENTITY_WOODEN_BOX) {
                    cursor_e->hatch.alive = true;
                    cursor_e->hatch.input = state->hatch_input;
                    cursor_e->hatch.rotation = state->cursor_rotation;
                }
            }
        }
    }

    if (IsKeyPressed(KEY_G))
        state->render_grid = !state->render_grid;

    if (IsKeyPressed(KEY_H)) {
        state->mode = MODE_HATCH_MOUNTING;
        state->cursor_rotation = 0;
    }

    if (IsKeyPressed(KEY_B)) {
        state->mode = state->mode == MODE_BUILDING ? MODE_INFO : MODE_BUILDING;
        state->cursor_rotation = 0;
    }

    if (IsKeyPressed(KEY_R))
        cycle_range(&state->cursor_rotation, 0, 4);

    if (IsKeyPressed(KEY_SPACE)) {
        if (state->mode == MODE_BUILDING) {
            cycle_range((int *) &state->building_type, 0, ENTITIES_TYPE_COUNT);
        } else if (state->mode == MODE_HATCH_MOUNTING) {
            state->hatch_input = !state->hatch_input;
        }
    }
}

void hatch_transfer_items(State *state) {
    World *world = &state->world;
    for (size_t i = 0; i < world->entities_data.size; ++i) {
        if (world->entities[i].type == ENTITY_WOODEN_BOX) {
            Entity *box = &world->entities[i];
            if (box->hatch.alive) {
                int x = box->pos.x + sides[box->hatch.rotation].x;
                int y = box->pos.y + sides[box->hatch.rotation].y;

                if (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_SIZE) continue;

                int idx = world->tiles[x][y].entity_idx;
                if (idx != -1) {
                    Entity *e = &world->entities[idx];
                    if (is_pipe(e->type)) {
                        Direction dir = directions[box->hatch.rotation];
                        Direction pipe_dir = rotate4(e->dir, 2);
                        if (!(pipe_dir & dir)) 
                            continue;
                    }
                    if (box->hatch.input && e->items_contained > 0) {
                        box->items_buffered += 1;
                        e->items_buffered -= 1;
                    }
                    if (!box->hatch.input && box->items_contained > 0) {
                        box->items_buffered -= 1;
                        e->items_buffered += 1;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < world->entities_data.size; ++i) {
        if (world->entities[i].items_buffered != 0) {
            Entity *e = &world->entities[i];
            e->items_contained += e->items_buffered;
            e->items_buffered = 0;
        }
    }
}

void cleanup(State *state) {
    free(state->world.entities);

    for (size_t i = 0; i < TEXTURES_SIZE; ++i)
        UnloadTexture(state->textures[i]);
}

int main() {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

    State state;
    state.textures[TEX_WOODEN_BOX]     = LoadTexture("sprites/wooden_box.png");
    state.textures[TEX_INPUT_HATCH]    = LoadTexture("sprites/input_hatch.png");
    state.textures[TEX_OUTPUT_HATCH]   = LoadTexture("sprites/output_hatch.png");
    state.textures[TEX_PIPE_TWO]       = LoadTexture("sprites/pipe_2.png");
    state.textures[TEX_PIPE_THREE]     = LoadTexture("sprites/pipe_3.png");
    state.textures[TEX_PIPE_FOUR]      = LoadTexture("sprites/pipe_4.png");
    state.textures[TEX_PIPE_CORNER]    = LoadTexture("sprites/pipe_corner.png");
    state.textures[TEX_CURSOR]         = LoadTexture("sprites/cursor.png");

    state.no_place_attempts = true;
    state.mode = MODE_INFO;
    state.cursor_rotation = 0;
    state.building_type = ENTITY_WOODEN_BOX;
    state.render_grid = false;
    state.hatch_input = true;
    init_world(&state.world);

    HideCursor();
    Rectangle src = { 0, 0, SPRITE_SIZE, SPRITE_SIZE };
    Vector2 origin = { TILE_SIZE / 2, TILE_SIZE / 2 };

    state.last_updated = 0.f;
    state.item_transfer_freq = 1.f;

    while (!WindowShouldClose()) {  
        Vector2 mouse_pos = GetMousePosition();
        process_input(&state, mouse_pos);
        float time = GetTime();
        while (time >= state.last_updated + state.item_transfer_freq) {
            hatch_transfer_items(&state);
            state.last_updated = time;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        render_entities(&state, src, origin);
        render_placement(&state, src, origin, mouse_pos);

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
Press H to place hatches\n\
Press G to render grid\n"; 
            DrawText(string, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, 20, WHITE);
        }

                                                                                
        Rectangle dest = { mouse_pos.x, mouse_pos.y, TILE_SIZE, TILE_SIZE };
        DrawTexturePro(state.textures[TEX_CURSOR], src, dest, origin, 0, WHITE);
        EndDrawing();
    }
    
    cleanup(&state);
    CloseWindow();
    return 0;
}
