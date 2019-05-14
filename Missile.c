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
    int speed;          
    int x;              
    int y;
    int targetX;        
    int targetY;
} emissile;

typedef struct emhead {     //enemy missiles list head
    struct emissile *next;
} emhead;

typedef struct fmhead {     //friendly missiles list head
    struct fmissile *next;
} fmhead;

void drawScene(WINDOW * win) {
    city cities[6];                          //creates array of cities
    battery batteries[3];                    //creates array of batteries
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_RED);    //initalises color pairs
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);

    wattron(win, COLOR_PAIR(1));
    for(int i = 1; i < WIDTH-1; i++)
        for(int j = 0; j < 5; j++)
            mvwprintw(win, 2+j+(HEIGHT/8)*7, i, " ");

    for(int i = 0; i < 1; i++) {
        batteries[i].id = i;
        batteries[i].y = (HEIGHT/8)*7;
        batteries[i].x = (WIDTH/7)*(i+1); 
        batteries[i].destroyed = false;
        batteries[i].stock = 10;

        mvwprintw(win, batteries[i].y - 4, WIDTH/2 - 3, "         ");
        mvwprintw(win, batteries[i].y - 3, WIDTH/2 - 4, "     Y     ");
        mvwprintw(win, batteries[i].y - 2, WIDTH/2 - 5, "     Y Y     ");
        mvwprintw(win, batteries[i].y - 1, WIDTH/2 - 6, "     Y Y Y     ");
        mvwprintw(win, batteries[i].y - 0, WIDTH/2 - 7, "     Y Y Y Y     ");
        mvwprintw(win, batteries[i].y + 1, WIDTH/2 - 7, "                 ");
    }
    wattron(win, COLOR_PAIR(2));
    for(int i = 0; i < 6; i++) {
        cities[i].id = i;
        cities[i].y = (HEIGHT/8)*7;
        cities[i].x = (WIDTH/7)*(i+1); 
        cities[i].destroyed = false;

        mvwprintw(win, cities[i].y, cities[i].x + 1, "     ");
        mvwprintw(win, cities[i].y + 1, cities[i].x - 2, "           ");
        wattron(win, A_UNDERLINE);
        mvwprintw(win, cities[i].y + 2, cities[i].x - 2, "           ");
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

void launchMissile(WINDOW *win, fmhead *fmp, cursor *c) {
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

        if(fmp->next != NULL)
            newmissile->next = fmp->next;
        fmp->next = newmissile;
    }
}

void moveMissiles(WINDOW *win, fmhead *fmp) {
    fmissile *node = fmp->next;
    while(node != NULL) {
        mvwprintw(win, (int)node->y, (int)node->x, " ");

        if(node->y > node->targetY)
            node->y -= node->speedY;

        if(node->x > node->targetX)
            node->x -= node->speedX;
        else if(node->x < node->targetX)
            node->x += node->speedX;

        if(node->y < node->targetY) {
            mvwprintw(win, (int)node->y, (int)node->x, " ");
        }
        else {
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, (int)node->y, (int)node->x, "o");
            wattroff(win, COLOR_PAIR(3));
        }
        node = node->next;
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

int main() {
    
    bool P = false;    //is game paused
    cursor c;          //creates cursor
    c.x = 128;         //set coordiantes to middle of window
    c.y = 32;
    cursor *pc = &c;   //pointer to cursor
    WINDOW *win;       //pointer to window
    fmhead *fhead = (fmhead*)malloc(sizeof(fmhead));
    emhead *ehead = (emhead*)malloc(sizeof(emhead));

    initscr();
    curs_set(0);                        //cursor invisible
    cbreak();
    win = gameWindow(HEIGHT, WIDTH);    //creates window/border
    drawScene(win);                     //sets initial stage
    keypad(win, TRUE);                  //enables key input
    mousemask(ALL_MOUSE_EVENTS, NULL);  //enable mouse input

    noecho();
    nodelay(win, true);

    while(!P) {                 //while unpaused
        moveCursor(pc, win);    //update cursor
        launchMissile(win, fhead, pc);
        moveMissiles(win, fhead);
        sleep(0.01);
        wrefresh(win);          //update windows
    }


    getch();
    endwin();
    return 0;
}


