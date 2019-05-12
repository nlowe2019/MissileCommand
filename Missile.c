#include <ncurses.h>

#define WIDTH 256
#define HEIGHT 64

typedef struct city {
    int id;
    int x;
    int y;
    bool destroyed;
} city;

typedef struct cursor {
    int x;
    int y;
} cursor;

typedef struct battery {
    int id;
    int x;
    int y;
    int stock;
    bool destroyed;
} battery;

typedef struct missile {
    int id;
    int speed;
    int x;
    int y;
} missile;

void drawScene(WINDOW * win) {
    city cities[6];
    battery batteries[3];
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_RED);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);

    wattron(win, COLOR_PAIR(1));
    for(int i = 1; i < WIDTH-1; i++)
        for(int j = 0; j < 2; j++)
            mvwprintw(win, 2+j+(HEIGHT/8)*7, i, " ");

    for(int i = 0; i < 3; i++) {
        batteries[i].id = i;
        batteries[i].y = (HEIGHT/8)*7;
        batteries[i].x = (WIDTH/7)*(i+1); 
        batteries[i].destroyed = false;
        batteries[i].stock = 10;
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

void moveCursor(cursor *pc, WINDOW *win) {
    int k = wgetch(win);
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, (*pc).y, (*pc).x, " ");
    wattron(win, COLOR_PAIR(3));
		switch(k)
		{	case KEY_UP:
                        if((*pc).y > 3)
                            (*pc).y -= 2;
                        break;
            case KEY_DOWN:
                        if((*pc).y < (HEIGHT/8)*7 - 5)
                            (*pc).y += 2;
                        break;
            case KEY_LEFT:
                        if((*pc).x > 6)
                            (*pc).x -= 4;
                        break;
            case KEY_RIGHT:
                        if((*pc).x < WIDTH - 6)
                            (*pc).x += 4;
                        break;
		}
    mvwprintw(win, (*pc).y, (*pc).x, "+");
    wattroff(win, COLOR_PAIR(3));
    wrefresh(win);
}

int main() {
    
    bool P = false;
    cursor c;
    c.x = 128;
    c.y = 32;
    cursor *pc = &c;
    WINDOW *win;

    initscr();
    curs_set(0);
    cbreak();
    win = gameWindow(HEIGHT, WIDTH);
    drawScene(win);
    keypad(win, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    while(!P) {
        moveCursor(pc, win);
        wrefresh(win);
    }

    getch();
    endwin();
    return 0;
}


