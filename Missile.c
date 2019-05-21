#include <ncurses.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define WIDTH 256       //width of window
#define HEIGHT 64       //height of window

typedef struct city {   //cities to protect
    int id;
    int x;
    int y;
    bool destroyed;     //has it been destoyed or not
} city;

typedef struct cursor { //used to direct missiles
    int x;              //x location on screen
    int y;              //y ^
} cursor;

typedef struct battery { //
    int id;
    int x;               //coordinates
    int y;
    int stock;           //remaining missiles
    bool destroyed;      // has it been destoryed
} battery;

typedef struct fmissile {   //player missiles
    struct fmissile *next;
    float speedmod;
    float speedX;             //speed on x axis
    float speedY;             //speed on y axis
    float x;                  //current coordinates
    float y;
    int targetX;            //target coordinates
    int targetY;
} fmissile;

typedef struct emissile {   //enemy missiles
    struct emissile *next;
    float speedmod;
    float speedX;             //speed on x axis
    float speedY;             //speed on y axis
    float x;                  //current coordinates
    float y;
    int targetX;            //target coordinates
    int targetY;
} emissile;

typedef struct emhead {     //enemy missiles list head
    struct emissile *next;
    int remaining;
} emhead;

typedef struct fmhead {     //friendly missiles list head
    struct fmissile *next;
} fmhead;

city cities[6];                 //creates array of cities
battery batteries[3];           //creates array of batteries
WINDOW *win;                    //pointer to window
int explosion[WIDTH][7*(HEIGHT/8)]; //array for each space on window

void drawScene() {
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_RED);    //initalises color pairs
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);

    wattron(win, COLOR_PAIR(1));
    for(int i = 1; i < WIDTH-1; i++)
        for(int j = 0; j < 5; j++)
            mvwprintw(win, 2+j+(HEIGHT/8)*7, i, ".");

    for(int i = 0; i < 1; i++) {
        batteries[i].id = i;
        batteries[i].y = (HEIGHT/8)*7;
        batteries[i].x = (WIDTH/7)*(i+1); 
        batteries[i].destroyed = false;
        batteries[i].stock = 10;

        mvwprintw(win, batteries[i].y - 4, WIDTH/2 - 3, ".........");
        mvwprintw(win, batteries[i].y - 3, WIDTH/2 - 4, ".....Y.....");
        mvwprintw(win, batteries[i].y - 2, WIDTH/2 - 5, ".....Y.Y.....");
        mvwprintw(win, batteries[i].y - 1, WIDTH/2 - 6, ".....Y.Y.Y.....");
        mvwprintw(win, batteries[i].y - 0, WIDTH/2 - 7, ".....Y.Y.Y.Y.....");
        mvwprintw(win, batteries[i].y + 1, WIDTH/2 - 7, ".................");
    }
    wattron(win, COLOR_PAIR(2));
    for(int i = 0; i < 6; i++) {
        cities[i].id = i;
        cities[i].y = (HEIGHT/8)*7;
        cities[i].x = (WIDTH/7)*(i+1); 
        cities[i].destroyed = false;

        mvwprintw(win, cities[i].y, cities[i].x + 1, "=====");
        mvwprintw(win, cities[i].y + 1, cities[i].x - 2, "===========");
        wattron(win, A_UNDERLINE);
        mvwprintw(win, cities[i].y + 2, cities[i].x - 2, "===========");
        wattroff(win, A_UNDERLINE);
    }
    wattroff(win, COLOR_PAIR(2));
    wrefresh(win);
}

WINDOW * gameWindow() {
    WINDOW *win = newwin(HEIGHT, WIDTH, 0, 0);
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
    refresh();
    wrefresh(win);
    return win;
}

void launchMissile(fmhead *fmp, cursor *c) {
    char k = wgetch(win);

    if(k == 'q') {
        fmissile *newmissile = (fmissile*)malloc(sizeof(fmissile));
        newmissile->x = 1 + WIDTH/2;
        newmissile->y = 7*(HEIGHT/8) - 5;
        newmissile->targetX = c->x;
        newmissile->targetY = c->y;
        newmissile->speedmod = 0.002;

        float dx = abs(newmissile->targetX - newmissile->x) / 2;
        float dy = abs(newmissile->targetY - newmissile->y);
        float length = sqrtf(dx*dx+dy*dy);

        
        newmissile->speedX = newmissile->speedmod * 2 * (dx/length);
        newmissile->speedY = newmissile->speedmod * (dy/length);
        if(newmissile->targetX < newmissile->x)
            newmissile->speedX = -(newmissile->speedX);

        if(fmp->next != NULL)
            newmissile->next = fmp->next;
        fmp->next = newmissile;
    }
    else
        return;
}

void createExplosion(int x, int y) {
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++) {
            explosion[(x-1) + i][(y-1) + j] = 1000000;
            wattron(win, COLOR_PAIR(0));
            mvwprintw(win, (x-1)+i, (y-1)+j, "*");
        }
}

void moveEntities(fmhead *fmp, emhead *emp) {
    fmissile *prevnode = NULL;
    fmissile *node = fmp->next;
    while(node != NULL) {
        wattron(win, COLOR_PAIR(3));
        mvwprintw(win, (int)node->y, (int)node->x, ".");

        node->y -= node->speedY;
        node->x += node->speedX;

        if(node->y < node->targetY) {
            mvwprintw(win, (int)node->y, (int)node->x, " ");
            createExplosion((int)node->x, (int)node->y);
            prevnode->next = node->next;
            node = node->next;
        }
        else {
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, (int)node->y, (int)node->x, "o");
            wattroff(win, COLOR_PAIR(3));
            prevnode = node;
            node = node->next;
        }
    }

    emissile *prevenode = NULL;
    emissile *enode = emp->next;
    while(enode != NULL) {
        wattron(win, COLOR_PAIR(6));
        mvwprintw(win, (int)enode->y, (int)enode->x, ".");

        enode->y += enode->speedY;
        enode->x += enode->speedX;

        if(enode->y > enode->targetY) {
            mvwprintw(win, (int)enode->y, (int)enode->x, " ");
            createExplosion((int)node->x, (int)node->y);
            prevenode->next = enode->next;
            enode = enode->next;
        }
        else {
            wattron(win, COLOR_PAIR(5));
            mvwprintw(win, (int)enode->y, (int)enode->x, "o");
            wattroff(win, COLOR_PAIR(5));
            prevenode = enode;
            enode = enode->next;
        }
        enode = enode->next;
    }

    for(int i = 0; i < WIDTH; i++)
        for(int j = 0; j < 7*(HEIGHT/8); j++)
            if(explosion[i][j] != 0) {
                explosion[i][j] -= 1;
                if(explosion[i][j] == 0)
                    mvwprintw(win, j, i, " ");
            }
}

void moveCursor(cursor *pc, WINDOW *win) {
    int k = wgetch(win);
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, (*pc).y, (*pc).x, " ");
    wattron(win, COLOR_PAIR(3));

	switch(k)
	{	case KEY_UP:
                    if(pc->y > 4)
                        pc->y -= 2;
                    break;
        case KEY_DOWN:
                    if(pc->y < (HEIGHT/8)*7 - 8)
                        pc->y += 2;
                    break;
        case KEY_LEFT:
                    if(pc->x > 6)
                        pc->x -= 4;
                    break;
        case KEY_RIGHT:
                    if(pc->x < WIDTH - 6)
                        pc->x += 4;
                    break;
        case ERR:
            break;
	}

    mvwprintw(win, (*pc).y, (*pc).x, "+");
    wattroff(win, COLOR_PAIR(3));
}

void launchEnemy(emhead *emp) {
    if(rand() % 70000 == 1 && emp->remaining != 0) {
        emissile *newmissile = (emissile*)malloc(sizeof(emissile));
        newmissile->x = 1 + (rand() % WIDTH - 2);
        newmissile->y = 1;
        newmissile->targetX = rand() % WIDTH;
        newmissile->targetY = (HEIGHT/8)*7;
        newmissile->speedmod = 0.0006;

        float dx = abs(newmissile->targetX - newmissile->x) / 2;
        float dy = abs(newmissile->targetY - newmissile->y);
        float length = sqrtf(dx*dx+dy*dy);

        newmissile->speedX = newmissile->speedmod * 2 * (dx/length);
        newmissile->speedY = newmissile->speedmod * (dy/length);
        if(newmissile->targetX < newmissile->x)
            newmissile->speedX = -(newmissile->speedX);

        if(emp->next != NULL)
            newmissile->next = emp->next;
        emp->next = newmissile;
    }
}

int main() {
    
    bool P = false;    //is game paused
    cursor c;          //creates cursor
    c.x = 128;         //set coordiantes to middle of window
    c.y = 32;
    
    fmhead *fhead = (fmhead*)malloc(sizeof(fmhead));
    emhead *ehead = (emhead*)malloc(sizeof(emhead));
    ehead->remaining = 200000;

    initscr();
    curs_set(0);                        //cursor invisible
    cbreak();
    win = gameWindow(HEIGHT, WIDTH);    //creates window/border
    drawScene();                        //sets initial stage
    keypad(win, TRUE);                  //enables key input
    mousemask(ALL_MOUSE_EVENTS, NULL);  //enable mouse input

    noecho();
    nodelay(win, true);

    while(!P) {                         //while unpaused
        moveCursor(&c, win);            //update cursor
        launchMissile(fhead, &c);
        launchEnemy(ehead);
        moveEntities(fhead, ehead);
        sleep(1/60);
        wrefresh(win);                  //update windows
    }

    getch();
    free(fhead);
    free(ehead);
    endwin();
    return 0;
}


