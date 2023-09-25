#include <curses.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "missile_command.h"
#include "sprites.h"

// ncurses window
WINDOW *win;            
#define WIDTH 256
#define HEIGHT 64

// game objects
game_data* game;
cursor* c;
city cities[6];
missile* player_missiles_head = NULL;
missile* enemy_missiles_head = NULL;
explosion* explosion_head = NULL;

int timer = 0; // 1 = 10ms

/*
sets color pairs, creates cities
*/
void init_game()
{
    initscr();
    curs_set(0);                        // set terminal cursor invisible
    cbreak();
    keypad(stdscr, TRUE);               // enables key input
    mousemask(ALL_MOUSE_EVENTS, NULL);  // enable mouse input
    noecho();
    nodelay(win, true);
    nodelay(stdscr, true);
    mouseinterval(0);

    /*
    initialise ncurses colors
    */
    use_default_colors();
    start_color();
    init_pair(BACKGROUND_COLOR, COLOR_BLACK, -1);
    init_pair(SOLID_RED, COLOR_RED, COLOR_RED);
    init_pair(SOLID_CYAN, COLOR_CYAN, COLOR_CYAN);
    init_pair(SOLID_GREEN, COLOR_GREEN, COLOR_GREEN);
    init_pair(SOLID_BLUE, COLOR_BLUE, COLOR_BLUE);
    init_pair(SOLID_YELLOW, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(SOLID_WHITE, COLOR_WHITE, COLOR_WHITE);
    init_pair(PLAYER_MISSILE_COLOR, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(TRAIL_COLOR, SOLID_RED, SOLID_RED);
    init_pair(CITY_COLOR, COLOR_BLUE, COLOR_BLUE);
    init_pair(CURSOR_COLOR, COLOR_WHITE, COLOR_WHITE);
    init_pair(TEXT_COLOR, COLOR_YELLOW, -1);
    init_pair(EXPLOSION_RED, COLOR_RED + 8, COLOR_RED + 8);
    init_pair(EXPLOSION_BLACK, COLOR_BLUE + 8, COLOR_BLUE + 8);
    init_pair(EXPLOSION_YELLOW, COLOR_YELLOW + 8, COLOR_YELLOW + 8);
    init_pair(BASE_COLOR, 94, 94);
    init_pair(SCORE_COLOR, SOLID_RED, SOLID_RED);

    win = game_window();
    
    /*
    initialise game objects
    */
    c = malloc(sizeof(*c));
    c->x = WIDTH/2;
    c->y = HEIGHT/2;

    game = malloc(sizeof(*game));
    game->level_num = 0;
    game->speed_modifier = 0.05;

    for(int i = 0; i < 6; i++)
    {
        cities[i].id = i;
        cities[i].y = (HEIGHT/8)*7;
        cities[i].x = (WIDTH/9)*((i%3)+1);
        cities[i].x = i < 3 ? cities[i].x : WIDTH - cities[i].x;

        cities[i].destroyed = false;
    }
}

void update_score(int points_earned)
{
    if(game->score % 10000 > (game->score + (points_earned * game->score_multiplier)) % 10000)
        game->reserve_cities++;
    game->score += (points_earned * game->score_multiplier);
}

void end_level_update()
{
    while(game->reserve_missiles > 0)
    {
        game->reserve_missiles--;
        update_score(5);

        draw_score();
        wrefresh(win);
        usleep(80000);
        
        if(game->reserve_missiles == 0 && game->missile_stock > 0)
        {
            game->missile_stock--;
            game->reserve_missiles = 10;
        }
    }

    for(int i = 0; i < 6; i++)
    {
        if(!cities[i].destroyed)
        { 
            usleep(400000);
            update_score(100);
            mvwprintw(win, cities[i].y - 1, cities[i].x - 4, "         ");
            mvwprintw(win, cities[i].y, cities[i].x - 4, "         ");
            mvwprintw(win, cities[i].y + 1, cities[i].x - 4, "         ");

            draw_score();
            wrefresh(win);
        }
    }
    
    sleep(1);
    new_level();
    flushinp();
}

/*
draws static scenery (battery, cities etc...)
*/
void draw_scene()
{
    // clear screen
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    for(int x = 1; x < WIDTH-1; x++) {
        for(int y = 1; y < HEIGHT-1; y++) {
            mvwprintw(win, y, x, " ");
        }
    }
    
    // draw landscape
    wattron(win, COLOR_PAIR(BASE_COLOR));
    for(int i = 1; i < WIDTH-1; i++) {
        for(int j = 2; j < 10; j++) {
            if(!(j >= 7 && i > 10 && i < WIDTH-10))
                mvwprintw(win, HEIGHT - j, i, " "); 
        }
    }

    // draw battery 
    int battery_y = HEIGHT - (HEIGHT/8);
    mvwprintw(win, battery_y - 1, WIDTH/2 - 5,              "              ");
    mvwprintw(win, battery_y - 0, WIDTH/2 - 12,       "                            ");
    mvwprintw(win, battery_y + 1, WIDTH/2 - 18,  "                                        ");

    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    mvwprintw(win, HEIGHT - 5, 4, "     ");  
    mvwprintw(win, HEIGHT - 5, 12, "     ");  
    
    // draw cities
    wattron(win, COLOR_PAIR(CITY_COLOR));
    for(int i = 0; i < 6; i++)
    {
        if(!cities[i].destroyed)
        {
            mvwprintw(win, cities[i].y - 1, cities[i].x - 3,       " "); mvwprintw(win, cities[i].y - 1, cities[i].x + 2, "  ");
            mvwprintw(win, cities[i].y + 0, cities[i].x - 3,  "  "); mvwprintw(win, cities[i].y + 0, cities[i].x + 1,  "   ");
            mvwprintw(win, cities[i].y + 1, cities[i].x - 4, "         ");
        }
        wattron(win, A_UNDERLINE);
        mvwprintw(win, cities[i].y + 2, cities[i].x - 4, "          ");
        wattroff(win, A_UNDERLINE);
    }
}

/*
changes color of entities at time intervals
*/
void animate_entities()
{
    if(timer % 10 == 0)
    {
        // change launch pad color
        int battery_y = (HEIGHT/8)*7;
        int rand_color = ((timer % 30)) / 10;
        switch (rand_color+1)
        {
            case 1:
                wattron(win, COLOR_PAIR(SOLID_WHITE)); 
                break;
            case 2:
                wattron(win, COLOR_PAIR(SOLID_CYAN)); 
                break;
            case 3:
                wattron(win, COLOR_PAIR(SOLID_BLUE)); 
                break; 
        }
        mvwprintw(win, battery_y - 2, WIDTH/2 - 0, "    ");
    }

    if(timer % 10 == 0)
    {
        // change missile color
        missile* missile_node = player_missiles_head;
        explosion* explosion_node = explosion_head;
        int missile_color;
        int explosion_color;
        int rand_color = (timer % 30) / 10;
        switch (rand_color+1)
        {
            case 1:
                missile_color = SOLID_RED;
                break;
            case 2:
                missile_color = SOLID_YELLOW;
                break;
            case 3:
                missile_color = SOLID_BLUE;
                break; 
        } 
        while(missile_node)
        {
            missile_node->color = missile_color;
            missile_node = missile_node->next;
        }

        rand_color = ((timer % 15)) / 5;
        switch (rand_color+1) 
        {
            case 1:
                explosion_color = EXPLOSION_RED;
                break;
            case 3:
                explosion_color = EXPLOSION_YELLOW;
                break;
            default:
                explosion_color = EXPLOSION_BLACK; 
                break;
        } 
        while(explosion_node)
        {
            explosion_node->color = explosion_color;
            explosion_node = explosion_node->next;
        }
    }
    
    wattron(win, COLOR_PAIR(CURSOR_COLOR));
    mvwprintw(win, c->y-1, c->x - 3, "▄");
}

/*
creates game window
*/
WINDOW * game_window()
{
    WINDOW *win = newwin(HEIGHT, WIDTH, 0, 0);
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);          //draws border
    refresh();                                     //refreshes stdscr
    return win;
}

/*
creates new explosion object and inserts into front of list
*/
void create_explosion(int x, int y) //records spaces around missile explosion
{
    int w = 21;
    int h = 9; 

    explosion* new_explosion = malloc(sizeof(*new_explosion));
    new_explosion->x = x;
    new_explosion->y = y;
    new_explosion->w = w;
    new_explosion->h = h;
    new_explosion->coords = malloc(sizeof(*new_explosion->coords)*w*h);
    new_explosion->animation_timer = 0;
    new_explosion->next = NULL;
    new_explosion->prev = NULL;

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            *(new_explosion->coords + (y*w) + x) = 0;
        }
    }

    if(explosion_head)
    {
        new_explosion->next = explosion_head;
        explosion_head->prev = new_explosion; 
    }
    explosion_head = new_explosion;
}

/*
distance function, used for explosion radius calculations
*/
double pythag(double x1, double y1, double x2, double y2) 
{
    return sqrt(
        fabs(pow(x1 - x2, 2)) +
        fabs(0x5*pow(y1 - y2, 2))
    );
}

/*
iterates explosion instances and updates tile counters
calls remove_explosion when all instance counters reduced to zero
*/
void update_explosions()
{
    explosion* node = explosion_head;
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
     
    while(node)
    {
        bool active = false; 

        int midx = node->w/2;
        int midy = node->h/2;
        int w = node->w;
        int h = node->h;

        int max_distance = pythag(0, 0, midx, midy);

        for (int y = 0; y < h; y++) { 
            for (int x = 0; x < w; x++)
            {
                /*
                inverse of distance from center used to create circular expanding explosion
                e.g.
                node->coords += [0,1,2,2,2,1,0,
                                 1,2,3,3,3,2,1,
                                 2,3,4,4,4,3,2,
                                 2,3,4,5,4,3,2,
                                 2,3,4,4,4,3,2,
                                 1,2,3,3,3,2,1,
                                 0,1,2,2,2,1,0]
                tile becomes active after certain threshold
                */
                double dist = max_distance - pythag(x, y, midx, midy);

                /*
                explosion expands, then shrinks once timer is reached
                */
                if(node->animation_timer < 200)
                    *(node->coords + (y*w) + x) += pow(dist > max_distance-3 ? max_distance-4 : dist, 2);
                else
                    *(node->coords + (y*w) + x) -= pow(8* ((max_distance + 1 - dist) < 3 ? 2 : (max_distance + 1 - dist)), 2);

                /*
                clears explosion tiles for redraw
                */
                chtype tile_color = mvwinch(win, node->y - midy + y, node->x - midx + x) & A_COLOR;
                tile_color >>= 8;
                if(tile_color >= EXPLOSION_RED && tile_color <= EXPLOSION_YELLOW)
                    mvwprintw(win, node->y - midy + y, node->x - midx + x, " ");

                if(*(node->coords + (y*w) + x) >= 0)
                    active = true;
            }
        }

        node->animation_timer++;
        if(!active)
            node = remove_explosion(node);
        else
            node = node->next;
    }

    if(!enemy_missiles_head && !explosion_head && game->enemies_remaining == 0)
        game->state = end_level;
}

/*
checks destroyed missile coordinates, set city to destroyed if hit
*/
bool check_city_collisions(int x)
{
    bool earn_points = true;

    for(int i = 0; i < 6; i++)
    {
        if(!cities[i].destroyed)
        {
            if(cities[i].x == x) 
            {
                cities[i].destroyed = true;
                earn_points = false;
            }
        }
    }

    if(WIDTH/2 == x)
    {
        earn_points = false;
        game->reserve_missiles = 0;
        if(game->missile_stock > 0)
            game->reserve_missiles = 10;
        game->missile_stock--;
    }
    
    return earn_points;
}

void draw_explosions()
{
    explosion* node = explosion_head;

    while(node != NULL)
    {
        wattron(win, COLOR_PAIR(node->color));

        int midx = node->w/2;
        int midy = node->h/2;
        int w = node->w;
        int h = node->h;

        for (int y = 0; y < h; y++) { 
            for (int x = 0; x < w; x++)
            {
                if(*(node->coords + (y*w) + x) > 1000 && node->x - midx + x > 0 && node->x - midx + x < WIDTH)
                    mvwprintw(win, node->y - midy + y, node->x - midx + x, "@");
            }
        }

        node = node->next;
    }
}

explosion* remove_explosion(explosion* node) //expl head = null if: freenode=head and freenode->next = null
{
    explosion* temp = node->next;

    if(!node->prev)
    {
        if(node->next)
        {
            node->next->prev = NULL;
            explosion_head = node->next;
        }
        else
        {
            explosion_head = NULL;
        }
    }
    else
    {
        if(node->next)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        else
        {
            node->prev->next = NULL;
        }
    }

    node->prev = NULL;
    node->next = NULL;
    free(node->coords);
    free(node);

    return temp;
} 

/*
clears missile trail
*/
void remove_trail(missile *em)
{
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    float lastX = em->x;
    float lastY = em->y;
    while(em->y > 1) {
        em->y -= em->speedY;
        em->x -= em->speedX;
        char ch = (char)mvwinch(win, (int)em->y, (int)em->x);
        if(ch == '.')
            mvwprintw(win, (int)em->y, (int)em->x, " ");
    }
    em->y = lastY;
    em->x = lastX;
}

missile* remove_missile(missile** list_head, missile* node)
{
    missile* temp = node->next;
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    mvwprintw(win, (int)node->y, (int)node->x, " ");
    create_explosion((int)node->x, (int)node->y);
    if(node->show_trail)
        remove_trail(node);

    if(!node->prev)
    {
        if(!node->next)
            *list_head = NULL;
        else
        {
            node->next->prev = NULL;
            *list_head = node->next;
        }
    }
    else
    {
        if(node->next)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        else
        {
            node->prev->next = NULL;
        }
    }

    node->prev = NULL;
    node->next = NULL;
    free(node);

    return temp;
}

/*
update position of missiles
remove and free destroyed missiles
draw missiles
*/
void update_player_missiles()
{ 
    missile* node = player_missiles_head;

    while(node != NULL)
    {
        // missile reaches target
        if((int) node->y <= node->targetY)
        {
            /*
            deletes missile from list and links previous and next nodes
            */
            node = remove_missile(&player_missiles_head, node);
        }
        else 
        {
            /*
            draw trail
            update missile position, player missile move in negetive Y direction
            draw missile in new position
            */
            node->show_trail ? wattron(win, COLOR_PAIR(TRAIL_COLOR)) : wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
            mvwprintw(win, (int)node->y, (int)node->x, ".");

            node->y -= node->speedY;
            node->x += node->speedX;

            wattron(win, COLOR_PAIR(node->color));
            mvwprintw(win, (int)node->y, (int)node->x, " ");

            node = node->next;
        }
    }
}
void update_enemy_missiles()
{
    missile* node = enemy_missiles_head;

    while(node)
    {
        // missile reaches target
        if((int) node->y > node->targetY || (char)mvwinch(win, (int)(node->y + node->speedY), (int)(node->x + node->speedX)) == '@')
        {
            if(node->y < node->targetY)
                update_score(25);

            node = remove_missile(&enemy_missiles_head, node);
        }
        else 
        {
            /*
            draw trail
            update missile position, player missile move in negetive Y direction
            draw missile in new position
            */
            node->show_trail ? wattron(win, COLOR_PAIR(TRAIL_COLOR)) : wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
            mvwprintw(win, (int)node->y, (int)node->x, ".");

            node->y += node->speedY;
            node->x += node->speedX;

            wattron(win, COLOR_PAIR(CITY_COLOR));
            mvwprintw(win, (int)node->y, (int)node->x, " ");

            node = node->next;
        }
    }
}

void move_cursor(int x, int y) //moves cursor across screen
{
    // clears previous cursor draw
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    chtype tile_color = mvwinch(win, c->y-1, c->x - 3) & A_COLOR;
    tile_color >>= 8;
    if(tile_color == CURSOR_COLOR)
        mvwprintw(win, c->y-1, c->x - 3, "     ");
    

    c->y += y; //adds y value input
    c->x += x; //adds x value
    if(c->y < 7 || c->y > (HEIGHT/8)*7 - 8) //moves cursor back if out of bounds
        c->y -= y;
    if(c->x <= 6 || c->x >= WIDTH - 6)
        c->x -= x;   
}

void launch_missile(int x, int y) //function creates new missile - adds to front of list
{
    missile *new_missile;
    if(game->reserve_missiles > 0)
    {
        new_missile = malloc(sizeof(*new_missile)); //allocates memory for missile
        new_missile->x = 1 + WIDTH/2;        //starting coordinates
        new_missile->y = 7*(HEIGHT/8) - 3;
        new_missile->targetX = x;            //target coordinates equal cursor position
        new_missile->targetY = y;         
        new_missile->speedmod = 1;           //speed modifier for missile
        new_missile->show_trail = false;
        new_missile->next = NULL;
        new_missile->prev = NULL;

        float dx = fabs(new_missile->targetX - new_missile->x) / 2; //(pythagoras) x distance
        float dy = fabs(new_missile->targetY - new_missile->y);     //y distance
        float length = sqrtf(dx*dx+dy*dy);                       //total length between coordinates
                
        new_missile->speedX = new_missile->speedmod * 2 * (dx/length);  //calcs horizontal speed from length ratio
        new_missile->speedY = new_missile->speedmod * (dy/length);      //vertical speed
        if(new_missile->targetX < new_missile->x)                       //reverse x speed if target to the left
            new_missile->speedX = -(new_missile->speedX);

        if(player_missiles_head)
        {
            player_missiles_head->prev = new_missile; 
            new_missile->next = player_missiles_head;
        }
        player_missiles_head = new_missile;

        game->reserve_missiles -= 1;
        if(game->reserve_missiles == 0) {
            game ->missile_stock -= 1;
            if(game->missile_stock > -1) {
                game->reserve_missiles = 10;
            }
        }
    }
}

void launch_enemy_wave()
{
    missile* new_enemy_missile;
    srand(time(0));
    if(timer % 200 == 0  &&  (rand() % 2) == 0)
    {
        srand(time(0) + 10);
        int wave_size = (timer % 800) / 200;

        for(int i = 0; i <= wave_size && game->enemies_remaining > 0; i++)
        { 
            srand(time(0) + i);
            new_enemy_missile = malloc(sizeof(*new_enemy_missile)); //allocates memory for new missile 
            new_enemy_missile->targetX = cities[rand() % 6].x;//new missile targets random city
            new_enemy_missile->targetY = (HEIGHT/8)*7;        //y level for cities
            new_enemy_missile->speedmod = game->speed_modifier;               //speed modifier
            new_enemy_missile->show_trail = true;
            new_enemy_missile->next = NULL;
            new_enemy_missile->prev = NULL;

            int spread = pow(( rand() % (int)sqrt(WIDTH/2) ) , 2);
            int x = new_enemy_missile->targetX + (rand() % 2 == 0 ? spread : -spread);
            if(x <= 0)
                x = 1;
            if(x >= WIDTH)
                x = WIDTH-1;
            new_enemy_missile->x = x;
            new_enemy_missile->y = 1;                         //places missile at top of screen

            float dx = abs(new_enemy_missile->targetX - new_enemy_missile->x) / 2; //pythagoras
            float dy = abs(new_enemy_missile->targetY - new_enemy_missile->y);
            float length = sqrtf(dx*dx+dy*dy);                         //distance between spawn and target
            new_enemy_missile->speedX = new_enemy_missile->speedmod * 2 * (dx/length); //sets horizontal speed based on ratio
            new_enemy_missile->speedY = new_enemy_missile->speedmod * (dy/length);     //sets vertical speed

            if(new_enemy_missile->targetX < new_enemy_missile->x)                      //reverse x speed if target to the left
                new_enemy_missile->speedX = -(new_enemy_missile->speedX);

            if(enemy_missiles_head)
            {
                enemy_missiles_head->prev = new_enemy_missile; 
                new_enemy_missile->next = enemy_missiles_head;
            }
            enemy_missiles_head = new_enemy_missile;  

            game->enemies_remaining--;
        }
    }
}

void launch_enemy()
{
    if(rand() % 400 == 1 && game->enemies_remaining != 0) //randomly creates new missiles if there are still some remaining
    {
        srand(time(0));
        missile *new_enemy_missile = malloc(sizeof(*new_enemy_missile)); //allocates memory for new missile
        new_enemy_missile->x = 1 + (rand() % WIDTH - 2);  //places missile at random x location
        new_enemy_missile->y = 7;                         //places missile at top of screen
        new_enemy_missile->targetX = cities[rand() % 6].x;//new missile targets random city
        new_enemy_missile->targetY = (HEIGHT/8)*7;        //y level for cities
        new_enemy_missile->speedmod = game->speed_modifier;               //speed modifier
        new_enemy_missile->show_trail = true;
        new_enemy_missile->prev = NULL;
        new_enemy_missile->next = NULL;

        float dx = abs(new_enemy_missile->targetX - new_enemy_missile->x) / 2; //pythagoras
        float dy = abs(new_enemy_missile->targetY - new_enemy_missile->y);
        float length = sqrtf(dx*dx+dy*dy);                         //distance between spawn and target

        new_enemy_missile->speedX = new_enemy_missile->speedmod * 2 * (dx/length); //sets horizontal speed based on ratio
        new_enemy_missile->speedY = new_enemy_missile->speedmod * (dy/length);     //sets vertical speed
        if(new_enemy_missile->targetX < new_enemy_missile->x)                      //reverse x speed if target to the left
            new_enemy_missile->speedX = -(new_enemy_missile->speedX);

        new_enemy_missile->next = enemy_missiles_head;    // insert new missile into front of list
        if(enemy_missiles_head)
            enemy_missiles_head->prev = new_enemy_missile;
        enemy_missiles_head = new_enemy_missile; 
    
        game->enemies_remaining--;
    }
}

void draw_string(char* string, int x, int y) 
{
    bool num = false;
    int row = 0;

    int len = strlen(string);

    for(int i = 0; i < len; i++) 
    {
        if(string[i] == ' ') {
            x += 6;
            continue;
        }

        int id = (int)string[i]; //-97 -48
        if(id < 97) {
            num = true;
            id -= 48;
        }
        else {
            id -= 97;
        }
        for(int h = 0; h < 5; h++)
        {
            if(num)
                row = num_sprites[(5*id) + h];
            else
                row = AZ_sprites[(5*id) + h];

            for(int w = 0; w < 8; w++) 
            {
                wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
                mvwprintw(win, y+h, x+w, " ");

                wattron(win, COLOR_PAIR(SCORE_COLOR));
                if(row >> (7-w) & 1)
                    mvwprintw(win, y+h, x+w, " ");
            }
        }
        x +=10;
    }
}

/*
draws logo sprites on start screen
calculates x position of bottom sprite row and switches
message at specified interval
*/
void draw_logo(int x, int y)
{ 
    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
    for(int i = x; i < x+7+(16*7); i++)
        for(int j = y+20; j < y+20+7; j++)
            mvwprintw(win, j, i, " ");
    
    /*
    uses timer to calc scroll x-offset
    */
    int scroll_msg_offset = (timer % ((7+(7*16)) * 32)) > ((7+(7*16)) * 16) ? (14*7) : 0;
    int x_scroll = timer % ((7+(7*16)) * 4);
    x_scroll /= 2;
    if(timer < 450)
        x_scroll = 0;

    for(int i = 0; i < 7; i++)
        for(int h = 0; h < 7; h++) 
        {
            int row_1 = logo_1[h + (i*7)];
            int row_2 = logo_2[h + (i*7)];
            int row_3_1 = logo_3[h + (i*7) + scroll_msg_offset];
            int row_3_2 = logo_3[h + ((i+7)*7) + scroll_msg_offset];

            for(int w = 0; w < 16; w++)
            {
                    /*
                    clears previous frame
                    */
                    wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
                    mvwprintw(win, y+h, x+w+(i*16), " ");
                    mvwprintw(win, y+h + 10, x+w+(i*16), " ");

                    /*
                    checks bit from sprite data (1 == red, 0 == blank)
                    */
                    wattron(win, COLOR_PAIR(SOLID_RED));
                    if(row_1 >> (15-w) & 1)
                        mvwprintw(win, y+h, x+w+(i*16), " ");
                    if(row_2 >> (15-w) & 1)
                        mvwprintw(win, y+h + 10, x+w+(i*16), " ");

                    /*
                    only draws scroll message between left and right bounds
                    */
                    wattron(win, COLOR_PAIR(SOLID_BLUE));
                    if(row_3_1 >> (15-w) & 1)
                        if(1+x+w-(16*7)+(i*8)+x_scroll > x && x+w-(16*7)+(i*8)+x_scroll < x+7+(7*16)-10)
                            mvwprintw(win, y+h + 20, 1+x+w-(16*7)+(i*8)+x_scroll, " ");
                    
                    if(row_3_2 >> (15-w) & 1)
                        if(1+x+w-(16*7)+((i+7)*8)+x_scroll > x && x+w-(16*7)+((i+7)*8)+x_scroll < x+7+(7*16)-10)
                            mvwprintw(win, y+h + 20, 1+x+w-(16*7)+((i+7)*8)+x_scroll, " ");
            }
        }
}

/*
redraw score counter and bottom-left game info
*/
void draw_score()
{
    /*
    update score/level count
    */
    wattron(win, COLOR_PAIR(TEXT_COLOR));
    mvwprintw(win, HEIGHT-1, 3, "LEVEL: %02d", game->level_num);
    mvwprintw(win, HEIGHT-1, 15, "SCORE: %06d", game->score);
    mvwprintw(win, HEIGHT-1, 31, "BONUS CITIES: %d", game->reserve_cities);

    char *str = malloc(6 * sizeof(*str)); 
    sprintf(str, "%d", game->score);
    int len = strlen(str); 
    draw_string(str, WIDTH/2 + (30) - (10*len), 1);
    free(str);

    /*
    update missile reserve count
    */
    for(int i = 0; i < 2; i++) {
        wattron(win, COLOR_PAIR(BASE_COLOR));
        mvwprintw(win, HEIGHT - 5, 4 + (i*8), "     ");
        wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
        if(game->missile_stock > i)
            mvwprintw(win, HEIGHT - 5, 4 + (i*8), "     ");   
    } 

    /*
    update current battery missile count 
    */
    for(int i = 0; i < 10; i++)
    {
        int row = i;
        int counter = 0;
        while(row >= 0)
        {
            counter++;
            row -= counter;
        }
        int cols = abs(row);
        row = counter -1;

        wattron(win, COLOR_PAIR(BASE_COLOR));
        mvwprintw(win, HEIGHT - 7 + row, WIDTH/2 + 8 - (row*4) + (row * 8)-(cols * 8), "    "); 
        wattron(win, COLOR_PAIR(BACKGROUND_COLOR));
        if(game->reserve_missiles > i)
            mvwprintw(win, HEIGHT - 7 + row, WIDTH/2 + 8 - (row*4) + (row * 8)-(cols * 8), "    "); 
    }
}

/*
increments level counter, increases missile speed, changes scene colors
*/
void new_level()
{
    int color_id = ((game->level_num /2 ) %8 );
    color_id *= 6;
    
    init_pair(BACKGROUND_COLOR, palette[color_id + 0], palette[color_id + 0]);
    init_pair(BASE_COLOR, palette[color_id + 1], palette[color_id + 1]);
    init_pair(CITY_COLOR, palette[color_id + 2], palette[color_id + 2]);
    init_pair(TRAIL_COLOR, palette[color_id + 3], palette[color_id + 3]);
    init_pair(SCORE_COLOR, palette[color_id + 4], palette[color_id + 4]);
    init_pair(CURSOR_COLOR, palette[color_id + 5], palette[color_id + 5]);
    
    game->level_num += 1;
    game->missile_stock = 2;
    game->reserve_missiles = 10;
    game->enemies_remaining = 20;
    game->state = in_level;
    if(game->level_num < 13)
        game->speed_modifier += 0.01;

    if(game->level_num < 2)
        game->score_multiplier = 1;
    else if(game->level_num < 4)
        game->score_multiplier = 2;
    else if(game->level_num < 6)
        game->score_multiplier = 3;
    else if(game->level_num < 8)
        game->score_multiplier = 4;
    else if(game->level_num < 10)
        game->score_multiplier = 5;
    else
        game->score_multiplier = 6;

    /*
    reserve cities used 
    */
    for(int i = 0; i < 6; i++) {
        if(cities[i].destroyed && game->reserve_cities > 0) {
            cities[i].destroyed = false;
            game->reserve_cities--;
        }
    }

    draw_scene();
}

/*
player controls

SPACE           | start game

WASD/ARROW-KEYS | trigger cursor movement
Q/SPACE         | launches missile
P               | pause game
BACKSPACE       | end game prematurely
*/
void player_input()
{
    int input = getch();
    MEVENT mouse_event;
    
    if(game->state == in_level)
        switch(input) 
        {   
            case KEY_UP: 
                move_cursor(0, -4);
                break;
            case 'w':
                move_cursor(0, -4);
                break;
            case KEY_DOWN:
                move_cursor(0, 4);
                break;
            case 's':
                move_cursor(0, 4);
                break;       
            case KEY_LEFT:
                move_cursor(-7, 0);
                break;
            case 'a':
                move_cursor(-7, 0);
                break;        
            case KEY_RIGHT:
                move_cursor(7, 0); 
                break;
            case 'd':
                move_cursor(7, 0);
                break;

            case 'q':
                launch_missile(c->x, c->y);   
                break;
            case ' ':                       
                launch_missile(c->x, c->y);   
                break; 

            case 'n':
                game->state = end_level;
                break;
            case 'p':
                game->state = halt;
                break;
            case KEY_BACKSPACE:
                game->state = game_over;
                break;
            case KEY_MOUSE:
                if(getmouse(&mouse_event) == OK)
                {
                    if(mouse_event.bstate & BUTTON1_PRESSED)
                    {
                        int x = mouse_event.x; int y = mouse_event.y;
                        if(y < 7)
                            y = 7;
                        else if(y > HEIGHT - (HEIGHT/8) - 8)
                            y = HEIGHT - (HEIGHT/8) - 8;
                        if(x < 8)
                            x = 8;
                        else if(x > WIDTH - 8)
                            x = WIDTH - 8; 
                        launch_missile(x, y);
                    }
                }
                break;
            default:
                break;
	    }

    else if(game->state == start_screen)
        switch(input) 
        {
            case ' ': 
                new_level();
                sleep(1);
                break;
        }

    else if(game->state == halt)
        switch(input) 
        {
            case 'p': 
                game->state = in_level;
                break;
        }

    else {
        ;
    }
}

int main()
{   
    init_game();
    wrefresh(win);
    usleep(500000);
    draw_scene();

    do
    {
        player_input(); 

        // start screen animation
        if(game->state == start_screen) 
        {
            draw_logo((WIDTH/2)-((7*16)/2)+2, (HEIGHT/2)-20);
        }

        // game logic
        else if(game->state == in_level)
        { 
            launch_enemy_wave(); 
            update_explosions(); 
            draw_explosions(); 
            update_player_missiles();
            update_enemy_missiles();
            animate_entities(); 
            draw_score();
        }

        // end of level animation
        else if(game->state == end_level)
        {
            end_level_update();
        }

        // display game score, end game
        if(game->state == game_over)
        {
            char *str = malloc(6 * sizeof(*str)); 
            sprintf(str, "%06d", game->score);
            draw_string("game over", (WIDTH/2)-(42), (HEIGHT/2)-5);
            draw_string(str, (WIDTH/2)-30, (HEIGHT/2)+1);
            free(str);
            free(c);
            free(game);
        }

        timer++;
        usleep(10000);          //creates delay before refresh
        wrefresh(win);                   //update windows
            

    } while(game->state != game_over);

    sleep(15);
    endwin();

    return 0;
}