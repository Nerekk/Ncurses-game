#include <time.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "game.h"

void worldinit(struct world* world)
{
    char** map = (char**)malloc(MAPHEIGHT * sizeof(char*));
    for (int i = 0; i < MAPHEIGHT; i++)
        map[i] = (char*)malloc(MAPWIDTH * sizeof(char));

    FILE *f = fopen("map.txt", "r");
    for (int i=0; i < MAPHEIGHT; i++)
    {
        for (int j=0; j < MAPWIDTH; j++)
        {
            fscanf(f, "%c", &map[i][j]);
        }
    }
    map[MAPHEIGHT/2][MAPWIDTH/2]='A';
    fclose(f);
    world->map = map;
    world->roundcounter=0;
    world->all_balance=0;
    world->all_carried_money=0;
    world->beastamount=0;
}
void freemap(struct world* w)
{
    if (w)
    {
        if (w->map)
        {
            char** map = w->map;
            for (int i=0; i<MAPHEIGHT; i++)
            {
                free(map[i]);
            }
            free(map);
        }
    }
}

void initplayerstructs(struct world* world)
{
    for (int i=0; i < MAXPLAYERS; i++)
    {
        world->players[i].name = i+1;
        world->players[i].carried_money = 0;
        world->players[i].balance = 0;
        world->players[i].status = 0;
        world->players[i].direction = STAY;
        world->players[i].campsite_x=-1;
        world->players[i].campsite_y=-1;
        world->players[i].deaths=0;
        world->players[i].bushwait=0;
        world->players[i].isbush=0;
    }
}
void free_coords(int* x, int* y, struct world* world)
{
    srand(time(NULL));
    int tempx, tempy;
    char** map = world->map;
    for (int i = 0; i < 200; i++)
    {
        tempx = rand() % MAPHEIGHT;
        tempy = rand() % MAPWIDTH;
        if (map[tempx][tempy] == ' ')
        {
            *x = tempx;
            *y = tempy;
            break;
        }
    }
}
void spawnmoney(struct world* world, enum moneytype type)
{
    int coiny;
    int coinx;
    char** map = world->map;
    for (int i=0; i < 200; i++)
    {
        coiny = rand() % MAPWIDTH;
        coinx = rand() % MAPHEIGHT;
        if (map[coinx][coiny] == ' ')
        {
            switch (type)
            {
                case COIN:
                    map[coinx][coiny] = 'c';
                    break;
                case TREASURE:
                    map[coinx][coiny] = 't';
                    break;
                case LARGE_TREASURE:
                    map[coinx][coiny] = 'T';
                    break;
                default:
                    return;
            }
            break;
        }
    }

}
void setbeastvision(struct beast* p, struct world* world)
{
    char** map = world->map;
    int x = p->x-2;
    int y = p->y-2;
    int k = 0;
    int l = 0;
    for (int i = x; i<x+5; i++)
    {
        for (int j = y; j<y+5; j++)
        {
            if ((i>=0 && i<MAPHEIGHT) && (j>=0 && j<MAPWIDTH))
            {
                p->tab[k][l] = map[i][j];
            }
            l++;
        }
        k++;
        l=0;
    }
}
int scanbeastmap(struct beast* p, int* move)
{
    int x=2;
    int y=2;
    int bush[4]={0, 0, 0, 0};
    for (int i=0; i<5; i++)
    {
        if (isplayer(p->tab[i][y]))
        {
            if (i<2)
            {
                *move = UP;
                return 1;
            }
            else
            {
                *move = DOWN;
                return 1;
            }
        }
        if (i<2 && p->tab[i][y]=='#') bush[0]=1;
        if (i>2 && p->tab[i][y]=='#') bush[1]=1;
        if (i<2 && p->tab[x][i]=='#') bush[2]=1;
        if (i>2 && p->tab[x][i]=='#') bush[3]=1;
        if (isplayer(p->tab[x][i]))
        {
            if (i<2)
            {
                *move = LEFT;
                return 1;
            }
            else
            {
                *move = RIGHT;
                return 1;
            }
        }
    }
    if (bush[0] && (isplayer(p->tab[0][1]) || isplayer(p->tab[0][3])))
    {
        *move = UP;
        return 1;
    }
    if (bush[1] && (isplayer(p->tab[4][1]) || isplayer(p->tab[4][3])))
    {
        *move = DOWN;
        return 1;
    }
    if (bush[2] && (isplayer(p->tab[1][0]) || isplayer(p->tab[3][0])))
    {
        *move = LEFT;
        return 1;
    }
    if (bush[3] && (isplayer(p->tab[1][4]) || isplayer(p->tab[3][4])))
    {
        *move = RIGHT;
        return 1;
    }


    *move=-1;
    return 0;
}

int iscoin(char c)
{
    if (c=='c' || c=='t' || c=='T') return 1;
    return 0;
}
int getcoin(char c)
{
    if (c=='c') return 1;
    if (c=='t') return 10;
    if (c=='T') return 50;
    return 0;
}
int isplayer(char c)
{
    if (c=='1') return 1;
    if (c=='2') return 2;
    if (c=='3') return 3;
    if (c=='4') return 4;
    return 0;
}

void printmap(struct world* world)
{
    char** map = world->map;
    for (int i = 0; i < MAPHEIGHT; i++)
    {
        for (int j = 0; j < MAPWIDTH; j++)
        {
            switch (map[i][j]) {
                case '#':
                    attron(COLOR_PAIR(WALL_PAIR));
                    mvaddch(i, j, ACS_CKBOARD);
                    attroff(COLOR_PAIR(WALL_PAIR));
                    break;
                case ' ':
                    attron(COLOR_PAIR(EMPTY_PAIR));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(EMPTY_PAIR));
                    break;
                case 'B':
                    attron(COLOR_PAIR(BUSH_PAIR));
                    mvaddch(i, j, ACS_CKBOARD);
                    attroff(COLOR_PAIR(BUSH_PAIR));
                    break;
                case 'A':
                    attron(COLOR_PAIR(CAMP_PAIR));
                    mvaddch(i, j, map[i][j]);
                    attroff(COLOR_PAIR(CAMP_PAIR));
                    break;
                case 'D':
                    attron(COLOR_PAIR(DROP_PAIR));
                    mvaddch(i, j, map[i][j]);
                    attroff(COLOR_PAIR(DROP_PAIR));
                    break;
                case '*':
                    attron(COLOR_PAIR(BEAST_PAIR));
                    mvaddch(i, j, map[i][j]);
                    attroff(COLOR_PAIR(BEAST_PAIR));
                    break;
                case 'c':
                case 't':
                case 'T':
                    attron(COLOR_PAIR(COIN_PAIR));
                    mvaddch(i, j, map[i][j]);
                    attroff(COLOR_PAIR(COIN_PAIR));
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                    attron(COLOR_PAIR(PLAYER_PAIR));
                    mvaddch(i, j, map[i][j]);
                    attroff(COLOR_PAIR(PLAYER_PAIR));
                    break;
            }
        }
    }
}
void printplayermap(struct player p)
{
    for (int i = 0; i < MAPHEIGHT; i++)
    {
        for (int j = 0; j < MAPWIDTH; j++)
        {
            switch (p.tab[i][j]) {
                case '0':
                    attron(COLOR_PAIR(FOG_PAIR));
                    mvaddch(i, j, ACS_CKBOARD);
                    attroff(COLOR_PAIR(FOG_PAIR));
                case '#':
                    attron(COLOR_PAIR(WALL_PAIR));
                    mvaddch(i, j, ACS_CKBOARD);
                    attroff(COLOR_PAIR(WALL_PAIR));
                    break;
                case ' ':
                    attron(COLOR_PAIR(EMPTY_PAIR));
                    mvaddch(i, j, ' ');
                    attroff(COLOR_PAIR(EMPTY_PAIR));
                    break;
                case 'B':
                    attron(COLOR_PAIR(BUSH_PAIR));
                    mvaddch(i, j, ACS_CKBOARD);
                    attroff(COLOR_PAIR(BUSH_PAIR));
                    break;
                case 'A':
                    attron(COLOR_PAIR(CAMP_PAIR));
                    mvaddch(i, j, p.tab[i][j]);
                    attroff(COLOR_PAIR(CAMP_PAIR));
                    break;
                case 'D':
                    attron(COLOR_PAIR(DROP_PAIR));
                    mvaddch(i, j, p.tab[i][j]);
                    attroff(COLOR_PAIR(DROP_PAIR));
                    break;
                case '*':
                    attron(COLOR_PAIR(BEAST_PAIR));
                    mvaddch(i, j, '*');
                    attroff(COLOR_PAIR(BEAST_PAIR));
                    break;
                case 'c':
                case 't':
                case 'T':
                    attron(COLOR_PAIR(COIN_PAIR));
                    mvaddch(i, j, p.tab[i][j]);
                    attroff(COLOR_PAIR(COIN_PAIR));
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                    attron(COLOR_PAIR(PLAYER_PAIR));
                    mvaddch(i, j, p.tab[i][j]);
                    attroff(COLOR_PAIR(PLAYER_PAIR));
                    break;
            }
        }
    }
    attron(COLOR_PAIR(WALL_PAIR));
    for (int j = 0; j < MAPHEIGHT; j++)
    {
        mvaddch(j, 0, ACS_CKBOARD);
        mvaddch(j, MAPWIDTH-1, ACS_CKBOARD);
    }
    for (int j = 0; j < MAPWIDTH; j++)
    {
        mvaddch(0, j, ACS_CKBOARD);
        mvaddch(MAPHEIGHT-1, j, ACS_CKBOARD);
    }
    attroff(COLOR_PAIR(WALL_PAIR));
}
void setplayervision(struct player* p, struct world* world)
{
    char** map = world->map;
    int x = p->x-2;
    int y = p->y-2;
    int k = 0;
    int l = 0;
    for (int i = 0; i < MAPHEIGHT; i++)
    {
        for (int j = 0; j < MAPWIDTH; j++)
        {
            p->tab[i][j] = 'O';
        }
    }
    for (int i = x; i<x+5; i++)
    {
        for (int j = y; j<y+5; j++)
        {
            if ((i>=0 && i<MAPHEIGHT) && (j>=0 && j<MAPWIDTH))
            {
                if (map[i][j]=='A')
                {
                    p->campsite_x=i;
                    p->campsite_y=j;
                }
                p->tab[i][j] = map[i][j];
            }
            l++;
        }
        k++;
        l=0;
    }

}
void printplayerstats(struct player p)
{
    attron(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(0, MAPWIDTH+4, "Player %c", (p.name+'0'));
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(1, MAPWIDTH+4, "Socket: %d", p.socket);
    mvprintw(3, MAPWIDTH+4, "Round: %d", p.roundcounter);

    mvprintw(5, MAPWIDTH+4, "Carried coins: %d", p.carried_money);
    mvprintw(6, MAPWIDTH+4, "Balance: %d", p.balance);
    mvprintw(7, MAPWIDTH+4, "Deaths: %d", p.deaths);
    mvprintw(9, MAPWIDTH+4, "Current [X Y]:  [%d %d]", p.y, p.x);
    if (p.campsite_x!=-1)
    {
        mvprintw(10, MAPWIDTH+4, "Campsite [X Y]: [%d %d]", p.campsite_y, p.campsite_x);
    }
    else
    {
        mvprintw(10, MAPWIDTH+4, "Campsite [X Y]: unknown");
    }
}
void printlegendplayer()
{
    mvprintw(13, MAPWIDTH+3, "Legend:");
    attron(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(14, MAPWIDTH+4, "1234");
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(14, MAPWIDTH+8, " - players");

    attron(COLOR_PAIR(WALL_PAIR));
    mvaddch(15, MAPWIDTH+4, ACS_CKBOARD);
    attroff(COLOR_PAIR(WALL_PAIR));
    mvprintw(15, MAPWIDTH+8, " - wall");

    attron(COLOR_PAIR(BUSH_PAIR));
    mvaddch(16, MAPWIDTH+4, ACS_CKBOARD);
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(16, MAPWIDTH+8, " - bushes");

    attron(COLOR_PAIR(BEAST_PAIR));
    mvprintw(17, MAPWIDTH+4, "*");
    attroff(COLOR_PAIR(BEAST_PAIR));
    mvprintw(17, MAPWIDTH+8, " - wild beast");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(18, MAPWIDTH+4, "c");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(18, MAPWIDTH+8, " - 1 coin");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(19, MAPWIDTH+4, "t");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(19, MAPWIDTH+8, " - 10 coins");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(20, MAPWIDTH+4, "T");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(20, MAPWIDTH+8, " - 50 coins");

    attron(COLOR_PAIR(DROP_PAIR));
    mvprintw(21, MAPWIDTH+4, "D");
    attroff(COLOR_PAIR(DROP_PAIR));
    mvprintw(21, MAPWIDTH+8, " - dropped coins");

    attron(COLOR_PAIR(CAMP_PAIR));
    mvprintw(22, MAPWIDTH+4, "A");
    attroff(COLOR_PAIR(CAMP_PAIR));
    mvprintw(22, MAPWIDTH+8, " - campsite");
}
void printlegend(struct world world)
{
    attron(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(1, MAPWIDTH+4, "Server");
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(3, MAPWIDTH+4, "Round: %d", world.roundcounter);

    mvprintw(5, MAPWIDTH+4, "Campsite [X Y]: [%d %d]", MAPWIDTH/2, MAPHEIGHT/2);
    mvprintw(6, MAPWIDTH+4, "Beast amount: %d", world.beastamount);

    mvprintw(13, MAPWIDTH+3, "Legend:");
    attron(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(14, MAPWIDTH+4, "1234");
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(14, MAPWIDTH+8, " - players");

    attron(COLOR_PAIR(WALL_PAIR));
    mvaddch(15, MAPWIDTH+4, ACS_CKBOARD);
    attroff(COLOR_PAIR(WALL_PAIR));
    mvprintw(15, MAPWIDTH+8, " - wall");

    attron(COLOR_PAIR(BUSH_PAIR));
    mvaddch(16, MAPWIDTH+4, ACS_CKBOARD);
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(16, MAPWIDTH+8, " - bushes");

    attron(COLOR_PAIR(BEAST_PAIR));
    mvprintw(17, MAPWIDTH+4, "*");
    attroff(COLOR_PAIR(BEAST_PAIR));
    mvprintw(17, MAPWIDTH+8, " - wild beast");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(18, MAPWIDTH+4, "c");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(18, MAPWIDTH+8, " - 1 coin");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(19, MAPWIDTH+4, "t");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(19, MAPWIDTH+8, " - 10 coins");

    attron(COLOR_PAIR(COIN_PAIR));
    mvprintw(20, MAPWIDTH+4, "T");
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(20, MAPWIDTH+8, " - 50 coins");

    attron(COLOR_PAIR(DROP_PAIR));
    mvprintw(21, MAPWIDTH+4, "D");
    attroff(COLOR_PAIR(DROP_PAIR));
    mvprintw(21, MAPWIDTH+8, " - dropped coins");

    attron(COLOR_PAIR(CAMP_PAIR));
    mvprintw(22, MAPWIDTH+4, "A");
    attroff(COLOR_PAIR(CAMP_PAIR));
    mvprintw(22, MAPWIDTH+8, " - campsite");
}
