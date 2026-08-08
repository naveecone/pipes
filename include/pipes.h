#ifndef _PIPES_H_
#define _PIPES_H_

#define WORLD_SIZE 30
#define TILE_SIZE 64
#define SPRITE_SIZE 32
#define INIT_ARR_SIZE 32
#define WINDOW_TITLE "Pipes"
#define WINDOW_WIDTH 1800
#define WINDOW_HEIGHT 900

#define ENTITIES_TYPE_COUNT 5
#define TEXTURES_SIZE 10

typedef enum {
    MODE_INFO,
    MODE_BUILDING,
    MODE_HATCH_MOUNTING
} Mode;

typedef struct {
    int x;
    int y;
} Vector2i;

typedef enum {
    // Cursors
    TEX_CURSOR_BUILD,
    TEX_CURSOR_INFO,
    TEX_TILE_CURSOR,

    // Hatches
    TEX_INPUT_HATCH,
    TEX_OUTPUT_HATCH,

    // Entities
    TEX_WOODEN_BOX,
    TEX_PIPE_TWO,
    TEX_PIPE_THREE,
    TEX_PIPE_FOUR,
    TEX_PIPE_CORNER,
} TextureType;

typedef enum {
    ENTITY_WOODEN_BOX,
    ENTITY_PIPE_TWO,
    ENTITY_PIPE_THREE,
    ENTITY_PIPE_FOUR,
    ENTITY_PIPE_CORNER
} EntityType;

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

typedef struct {
    Direction dir;
    Vector2i offset;
} Side; 

typedef struct {
    bool alive;
    bool input;
    int rotation;
} Hatch;

typedef struct {
    EntityType type;
    PipeType ptype;
    Vector2i pos;

    Direction dir;
    Direction last_input_dir;
    int rotation;
    int next_side;

    bool has_prev_input;
    Vector2 prev_input;

    Hatch hatch;
    int items_contained;
    // To prevent one item from being transferred multiple times in one tick
    int items_received_this_it;
} Entity;

typedef struct {
    int entity_idx;
} Tile;

typedef struct {
    size_t size;
    size_t cap;
} ArrData;

typedef struct {
    Tile tiles[WORLD_SIZE][WORLD_SIZE];

    Entity *entities;
    ArrData entities_data;
} World;

typedef struct {
    World world;
    Texture textures[TEXTURES_SIZE];
    
    int cursor_rotation;
    EntityType building_type;

    Mode mode;
    bool render_grid;
    bool no_place_attempts;
    bool hatch_input;

    float item_transfer_freq;
    float last_updated;
} State;

void init_world(World *world);

void print_direction_bits(Direction dir);

int rotate4(Direction dir, int n);

void add_entity(World *world, Entity e);

Vector2i get_tile_pos(Vector2 pos);

void cycle_range(int *target, int from, int to);

void print_entity_info(Entity e);

void render_cursor(State *state, Rectangle tex_src, Vector2 tex_origin, Vector2 pos);

void render_entities(State *state, Rectangle tex_src, Vector2 origin);

void render_placement(State *state, Rectangle tex_src, Vector2 tex_origin, Vector2 mouse_pos);

bool is_pipe(EntityType type);

bool outside_world(int x, int y);

void handle_info_click(State *state, int entity_idx);

void handle_building_click(State *state, Vector2i tile_pos);

void handle_hatch_click(State *state, int entity_idx);

void process_input(State *state, Vector2 mouse_pos);

void update_boxes(World *world);

void update_pipes(World *world);

void load_textures(State *state);

void cleanup(State *state);

#endif
