#include <curses.h>
#include <ncurses.h>

/*
objective to defend, incoming missiles 
target location
*/
typedef struct city {   
    int id;
    int x;
    int y;
    bool destroyed;
} city;

/*
player controlled cursor,
player missiles target position
*/
typedef struct cursor {
    int x;
    int y;
} cursor;

/*
projectiles
*/
typedef struct missile {
    struct missile *next;
    struct missile *prev;
    float speedmod;           // speed modifier
    float speedX;             // speed on x axis
    float speedY;             // speed on y axis
    float x;                  // co-ordinates
    float y;
    int targetX;              // target co-ordinates
    int targetY;
    int color;
    bool show_trail;          // draw trail behind missile
} missile;

/*
created on missile destruction, incoming missiles
destroyed on collision
*/
typedef struct explosion {
    struct explosion *next;     // bi-directional linked list
    struct explosion *prev;
    int x;                      // co-ordinates
    int y;
    int w;                      // dimensions
    int h;
    double* coords;             // tiles of explosion ( size: W x H)
    int animation_timer;
    int color;
} explosion;

enum game_state {
    start_screen,
    in_level,
    end_level,
    game_over,
    halt
};

/*
stores game data
*/
typedef struct game_data {

    int level_num;              // level number
    int score;                  // missile = 25; leftover_supply = 50; city = 100;
    int reserve_cities;
    int score_multiplier;       // increases score gain at higher levels

    int enemies_remaining;      // incoming missiles remaining in level
    float speed_modifier;       // modifies enemy speed, increases at higher levels
    int reserve_missiles;       // missiles remaining in battery (0-10)
    int missile_stock;          // stockpiles remaining (10 missiles each)

    enum game_state state;

} game_data;

/*
color pair names
*/
enum color_id {
    not_valid,
    SOLID_RED,
    SOLID_CYAN,
    SOLID_GREEN,
    SOLID_BLUE,
    SOLID_YELLOW,
    SOLID_WHITE,
    PLAYER_MISSILE_COLOR,
    TRAIL_COLOR,
    CITY_COLOR,
    CURSOR_COLOR,
    TEXT_COLOR,
    BACKGROUND_COLOR,
    EXPLOSION_RED,
    EXPLOSION_BLACK,
    EXPLOSION_YELLOW,
    BASE_COLOR,
    SCORE_COLOR,
};

WINDOW* game_window();
void new_level();

void create_explosion_new(int x, int y);
explosion* remove_explosion(explosion* node);

void update_score(int points_earned);
void draw_score();
void draw_string(char* string, int x, int y);