#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define WIDTH 256       //width of window
#define HEIGHT 64       //height of window

typedef struct city {   //cities
    int id;
    int x;
    int y;
    bool destroyed;     //has it been destoyed or not
} city;

typedef struct cursor { //controlled by player to direct missiles
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
    struct fmissile *prev;
    float speedmod;
    float speedX;             //speed on x axis
    float speedY;             //speed on y axis
    float x;                  //current coordinates
    float y;
    int targetX;            //target coordinates
    int targetY;
} fmissile;

typedef struct emissile {   //enemy missiles
    struct emissile *next; //pointer to next list missile
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
    int remaining;          //how many enemy missiles remaining
} emhead;

typedef struct fmhead {     //friendly missiles list head
    struct fmissile *next;
} fmhead;

city cities[6];                     //creates array of cities
battery batteries[3];               //creates array of batteries
WINDOW *win;                        //pointer to window
int explosion[WIDTH][HEIGHT];       //array for each space on window
bool endgame = false;
int score = 0;

void drawScene() { //function sets up game by drawing basic scenery
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_RED);    //initalises color pairs
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);

    wattron(win, COLOR_PAIR(1));           //draws landscape
    for(int i = 1; i < WIDTH-1; i++)
        for(int j = 0; j < 4; j++)
            mvwprintw(win, 2+j+(HEIGHT/8)*7, i, ".");

    for(int i = 0; i < 1; i++) {            //initialises batteries
        batteries[i].id = i;
        batteries[i].y = (HEIGHT/8)*7;
        batteries[i].x = (WIDTH/7)*(i+1); 
        batteries[i].destroyed = false;
        batteries[i].stock = 10;

        //draws battery
        mvwprintw(win, batteries[i].y - 4, WIDTH/2 - 3,     ".........");
        mvwprintw(win, batteries[i].y - 3, WIDTH/2 - 4,    "...........");
        mvwprintw(win, batteries[i].y - 2, WIDTH/2 - 5,   ".............");
        mvwprintw(win, batteries[i].y - 1, WIDTH/2 - 6,  "...............");
        mvwprintw(win, batteries[i].y - 0, WIDTH/2 - 7, ".................");
        mvwprintw(win, batteries[i].y + 1, WIDTH/2 - 7, ".................");
    }
    wattron(win, COLOR_PAIR(2)); //initialises/draws cities
    for(int i = 0; i < 6; i++) { //for loop to initialise each city
        cities[i].id = i;
        cities[i].y = (HEIGHT/8)*7;
        cities[i].x = (WIDTH/7)*(i+1); 
        cities[i].destroyed = false;

        mvwprintw(win, cities[i].y, cities[i].x + 1,        "=====");
        mvwprintw(win, cities[i].y + 1, cities[i].x - 2, "===========");
        wattron(win, A_UNDERLINE);
        mvwprintw(win, cities[i].y + 2, cities[i].x - 2, "===========");
        wattroff(win, A_UNDERLINE);
    }
    wattroff(win, COLOR_PAIR(2));
    mvwprintw(win, HEIGHT-1, 3, "SCORE: %06d", score); //prints score
}

WINDOW * gameWindow() {     //creates new window, returns pointer
    WINDOW *win = newwin(HEIGHT, WIDTH, 0, 0);     //creates new window
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);          //draws border
    refresh();                                     //refreshes stdscr
    return win;
}

void createExplosion(int x, int y) { //records spaces around missile explosion

    int count = 0;      //counts destroyed cities
    for(int i = 0; i < 5; i++)
        for(int j = 0; j < 4; j++) {
            explosion[(x-1) + i][(y-1) + j] = 500; //sets certain space to int (timer for how long it lasts)
            wattron(win, COLOR_PAIR(0));
            mvwprintw(win, (y-1)+j, (x-1)+i, "*"); //print * at location
            for(int k = 0; k < 6; k++) {
                if(cities[k].x == x && cities[k].y == y) //if explosion at same spot as city then destroy it
                    cities[k].destroyed == true;
                if(cities[k].destroyed == true)
                    count++;     //if city is destroyed add to count
                    if(count == 6) {  //if all cities destoryed then game over
                        endgame = true;
                    }
            }
        }
}

void removeFTrail(fmissile *fm) { //function moves missiles backwards to original spawn location, deleting trail
    while(fm->y < 7*(HEIGHT/8) - 5) { //while it's above original spawn location
        fm->y += fm->speedY;    //moves backwards
        fm->x -= fm->speedX;
        char ch = mvwinch(win, (int)fm->y, (int)fm->x); //retrieves characters along trail
        if(ch == '.')   //if part of trail then is replaced with blank space
            mvwprintw(win, (int)fm->y, (int)fm->x, " ");
    }
    fm->y = fm->targetY;
    fm->x = fm->targetX;
}

void removeETrail(emissile *em) { //behaves same as previous function but for enemy missiles
    float lastX = em->x;
    float lastY = em->y;
    while(em->y > 3) {
        em->y -= em->speedY;
        em->x -= em->speedX;
        char ch = mvwinch(win, (int)em->y, (int)em->x);
        if(ch == '.')
            mvwprintw(win, (int)em->y, (int)em->x, " ");
    }
    em->y = lastY;
    em->x = lastX;
}

void moveEntities(fmhead *fmp, emhead *emp) {   //function updates positions of all missiles
    fmissile *node = fmp->next;     //node points to first on list
    while(node != NULL) {   //while node is not NULL

        if(node->y < node->targetY) { //if missile destroyed
            mvwprintw(win, (int)node->y, (int)node->x, " "); //remove missile character
            createExplosion((int)node->x, (int)node->y);    //create explosion at last coordinates
            removeFTrail(node);     //remove missiles trail
            
            fmissile *freenode = node;  //temporary node to free memory
            node = node->next;  //node points to next on list
            if(fmp->next == freenode) {  //if node to be freed is first on list
                fmp->next = freenode->next;  //head points to second node instead
            }
            freenode->next = NULL;
            //free(freenode);
        }
        else {
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, (int)node->y, (int)node->x, ".");    //draws trail
            node->y -= node->speedY;    //increments poisiton by speed values
            node->x += node->speedX;
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, (int)node->y, (int)node->x, "o");    //draws missile in new location
            wattroff(win, COLOR_PAIR(3));

            node = node->next;  //node pointer points to next on list
        }
    }

    emissile *enode = emp->next;
    while(enode != NULL) { //while node is not NULL
        if(enode->y > enode->targetY || explosion[(int)enode->x][(int)enode->y] > 0) { //if missile is blown up
            mvwprintw(win, (int)enode->y, (int)enode->x, " "); //remove missile character
            createExplosion((int)enode->x, (int)enode->y);  //create explosion around missiles last coordinates
            removeETrail(enode);    //remove missiles trail
            
            emissile *freenode = enode;  //temporary pointer to free node
            enode = enode->next;         //point to next node on list
            if(emp->next == freenode) {     //if first node on list
                emp->next = enode; //head points to second node
            }
            freenode->next = NULL;
            //free(freenode);   //free memory

            score += 25;    //increase score on missile destruction
            mvwprintw(win, HEIGHT-1, 3, "SCORE: %06d", score);  //print new score
        }
        else { //if not blown up
            wattron(win, COLOR_PAIR(6));
            mvwprintw(win, (int)enode->y, (int)enode->x, "."); //draws trail
            enode->y += enode->speedY;  //increments x and y by speed values
            enode->x += enode->speedX;
            wattron(win, COLOR_PAIR(5));
            mvwprintw(win, (int)enode->y, (int)enode->x, "o"); //draws missile at new position
            wattroff(win, COLOR_PAIR(5));

            enode = enode->next; //sets node pointer to next node
        }
        
    }

    for(int i = 0; i < WIDTH; i++)  //reduces "timer" on each exploding square
        for(int j = 0; j < HEIGHT; j++)
            if(explosion[i][j] != 0) {
                explosion[i][j] -= 1;
                if(explosion[i][j] <= 0)
                    mvwprintw(win, j, i, " "); //prints blank space if timer = 0
            }
}

void moveCursor(cursor *pc, int x, int y) { //moves cursor across screen
    wattron(win, COLOR_PAIR(4));    
    mvwprintw(win, (*pc).y, (*pc).x, " ");  //deletes previous cursor character
    wattron(win, COLOR_PAIR(3));

    pc->y += y; //adds y value input
    pc->x += x; //adds x value
    if(pc->y < 4 || pc->y > (HEIGHT/8)*7 - 8) //moves cursor back if out of bounds
        pc->y -= y;
    if(pc->x <= 6 || pc->x >= WIDTH - 6)
        pc->x -= x;   

    mvwprintw(win, pc->y, pc->x, "+"); //prints cursor character
    wattroff(win, COLOR_PAIR(3));
}

void launchMissile(fmhead *fmp, cursor *c) { //function creates new missile - adds to front of list

    fmissile *newmissile = (fmissile*)malloc(sizeof(fmissile)); //allocates memory for missile
    newmissile->x = 1 + WIDTH/2;        //starting coordinates
    newmissile->y = 7*(HEIGHT/8) - 5;
    newmissile->targetX = c->x;         //target coordinates equal cursor position
    newmissile->targetY = c->y;         
    newmissile->speedmod = 0.1;         //speed modifier for missile

    float dx = abs(newmissile->targetX - newmissile->x) / 2; //(pythagoras) x distance
    float dy = abs(newmissile->targetY - newmissile->y);     //y distance
    float length = sqrtf(dx*dx+dy*dy);                       //total length between coordinates
            
    newmissile->speedX = newmissile->speedmod * 2 * (dx/length);  //calcs horizontal speed from length ratio
    newmissile->speedY = newmissile->speedmod * (dy/length);      //vertical speed
    if(newmissile->targetX < newmissile->x)                       //reverse x speed if target to the left
        newmissile->speedX = -(newmissile->speedX);
    
    newmissile->next = NULL;        //sets next pointer equal to NULL
    if(fmp->next != NULL)           //if list isnt empty then point to first node
        newmissile->next = fmp->next;
    fmp->next = newmissile;         //point head to new node
}

void launchEnemy(emhead *emp) {
    if(rand() % 400 == 1 && emp->remaining != 0) {  //randomly creates new missiles if there are still some remaining
        srand(time(0));
        emissile *newemissile = (emissile*)malloc(sizeof(emissile)); //allocates memory for new missile
        newemissile->x = 1 + (rand() % WIDTH - 2);  //places missile at random x location
        newemissile->y = 1;                         //places missile at top of screen
        newemissile->targetX = (rand() % 2) + cities[rand() % 6].x;//new missile targets random city
        newemissile->targetY = (HEIGHT/8)*7;        //y level for cities
        newemissile->speedmod = 0.05;               //speed modifier

        float dx = abs(newemissile->targetX - newemissile->x) / 2; //pythagoras
        float dy = abs(newemissile->targetY - newemissile->y);
        float length = sqrtf(dx*dx+dy*dy);                         //distance between spawn and target

        newemissile->speedX = newemissile->speedmod * 2 * (dx/length); //sets horizontal speed based on ratio
        newemissile->speedY = newemissile->speedmod * (dy/length);     //sets vertical speed
        if(newemissile->targetX < newemissile->x)                      //reverse x speed if target to the left
            newemissile->speedX = -(newemissile->speedX);

        newemissile->next = NULL;           //points to nothing
        if(emp->next != NULL)               //points to previous first node if there is one
            newemissile->next = emp->next;
        emp->next = newemissile;            //points head to new missile

        emp->remaining -= 1;
    }
}

void playerInput(int k, fmhead *fhead, cursor *pc) {
    switch(k) 
        {   
            case KEY_UP:                    //if up arrow key is pressed 
                moveCursor(pc, 0, -2);       //move cursor up
                break;
            case KEY_DOWN:
                moveCursor(pc, 0, 2);      //if down arrow key is pressed 
                break;        
            case KEY_LEFT:
                moveCursor(pc, -4, 0);      //if left arrow key is pressed 
                break;        
            case KEY_RIGHT:
                moveCursor(pc, 4, 0);       //if right arrow key is pressed 
                break;
            case 'q':                       //if Q is pressed then launch missile
                launchMissile(fhead, pc);   
                break;  
            case 'e':
                endgame = true;
                break;
            case ERR:                       //break if nothing pressed
                break;
	    }
}

int main() {
    
    bool P = false;    //is game paused
    cursor c;          //creates cursor
    c.x = 128;         //set coordiantes to middle of window
    c.y = 32;
    
    fmhead fhead;           //creates list headers
    fhead.next = NULL;      //initialises vars
    emhead ehead;
    ehead.next = NULL;
    ehead.remaining = 12;

    initscr();
    int k;
    curs_set(0);                        //cursor invisible
    cbreak();
    win = gameWindow(HEIGHT, WIDTH);    //creates window/border
    drawScene();                        //sets initial stage
    keypad(stdscr, TRUE);                  //enables key input
    mousemask(ALL_MOUSE_EVENTS, NULL);  //enable mouse input

    noecho();
    nodelay(win, true);
    nodelay(stdscr, true);

    while(!endgame) {                          //while unpaused
        k = getch();
        playerInput(k, &fhead, &c);       //uses key inputs
        launchEnemy(&ehead);              //laucnhes enemy missiles
        moveEntities(&fhead, &ehead);      //updates positions
        usleep(10000);                   //creates delay before refresh
        wrefresh(win);                   //update windows
    }

    mvwprintw(win, HEIGHT/2, WIDTH/2 - 8, "Game Over - Score: %d", score);
    wrefresh(win);
    sleep(50);

    endwin();
    return 0;
}


