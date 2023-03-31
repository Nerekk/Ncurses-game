#include <time.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "game.h"
#define PORT 8080
int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

World world;
int dropmap[MAPHEIGHT][MAPWIDTH];
int server_status;
pthread_t serverloop;
pthread_t serverkeyboard;
pthread_t clientinitializator;
pthread_mutex_t lock;

void socket_server_start()
{
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    server_status=1;
}
void endgame()
{
    server_status=0;
    for (int i=0; i<MAXPLAYERS; i++)
    {
        if (world.players[i].status)
        {
            pthread_cancel(world.players[i].player_thread);
        }
    }

    pthread_cancel(clientinitializator);
    freemap(&world);
    endwin();
    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
}
void* clientinit(void* arg)
{
    int clientsocket;
    while (server_status)
    {
        if ((clientsocket
                     = accept(server_fd, (struct sockaddr*)&address,
                              (socklen_t*)&addrlen))
            < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        int i;
        int check=0;
        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (world.players[i].status==0)
            {
                check=1;
                send(clientsocket, (void*) &check, sizeof(check), 0);
                world.players[i].status = 1;
                world.players[i].socket = clientsocket;
                free_coords(&world.players[i].start_x, &world.players[i].start_y, &world);
                world.players[i].x = world.players[i].start_x;
                world.players[i].y = world.players[i].start_y;
                world.map[world.players[i].x][world.players[i].y] = world.players[i].name + '0';
                pthread_create(&world.players[i].player_thread, NULL, client_move_handler, (void*)&world.players[i]);
                world.map[world.players[i].start_x][world.players[i].start_y] = world.players[i].name + '0';
                break;
            }
        }
        if (!check)
        {
            send(clientsocket, (void*) &check, sizeof(check), 0);
        }
    }
    return NULL;
}
void* server_keyboard(void* arg)
{
    int key;
    while (server_status)
    {
        key = getch();
        switch (key) {
            case 'q':
            case 'Q':
                endgame();
                break;
            case 'c':
                spawnmoney(&world, COIN);
                break;
            case 't':
                spawnmoney(&world, TREASURE);
                break;
            case 'T':
                spawnmoney(&world, LARGE_TREASURE);
                break;
            case 'b':
            case 'B':
                initbeast();
                break;
            default:
                break;
        }
    }
    return NULL;
}
void initbeast()
{
    for (int i=0; i < MAXBEASTS; i++)
    {
        if (world.beasts[i].status==0)
        {
            world.beasts[i].status=1;
            free_coords(&world.beasts[i].x, &world.beasts[i].y, &world);
            world.beasts[i].direction=STAY;
            world.beasts[i].bushwait=0;
            world.beasts[i].status=1;
            world.beasts[i].name='*';
            world.beasts[i].laststep=' ';
            world.beastamount++;
            world.map[world.beasts[i].x][world.beasts[i].y] = '*';
            break;
        }
    }
}
void movebestia(int i, int x, int y, int move)
{
    char step;
    if (world.map[x][y] == ' ') {
        step = world.map[x][y];
        world.map[world.beasts[i].x][world.beasts[i].y] = world.beasts[i].laststep;
        world.beasts[i].laststep = step;
        world.beasts[i].bushwait = 0;
        world.map[x][y] = world.beasts[i].name;
    } else if (world.map[x][y] == 'B') {
        step = world.map[x][y];
        world.map[world.beasts[i].x][world.beasts[i].y] = world.beasts[i].laststep;
        world.beasts[i].laststep = step;
        world.beasts[i].bushwait = 1;
        world.map[x][y] = world.beasts[i].name;
    } else if (iscoin(world.map[x][y])) {
        step = world.map[x][y];
        world.map[world.beasts[i].x][world.beasts[i].y] = world.beasts[i].laststep;
        world.beasts[i].laststep = step;
        world.beasts[i].bushwait = 0;
        world.map[x][y] = world.beasts[i].name;
    } else if (world.map[x][y] == 'D') {
        step = world.map[x][y];
        world.map[world.beasts[i].x][world.beasts[i].y] = world.beasts[i].laststep;
        world.beasts[i].laststep = step;
        world.map[x][y] = world.beasts[i].name;
    } else if (isplayer(world.map[x][y])) {
        int playerindex = isplayer(world.map[x][y]) - 1;
        world.all_carried_money -= world.players[playerindex].carried_money;
        killplayer(&world.players[playerindex]);
        step = world.map[x][y];
        world.map[world.beasts[i].x][world.beasts[i].y] = world.beasts[i].laststep;
        world.beasts[i].laststep = step;
        world.map[x][y] = world.beasts[i].name;
    }
    else {
        return;
    }
    switch (move) {
        case RIGHT:
            world.beasts[i].y++;
            break;
        case LEFT:
            world.beasts[i].y--;
            break;
        case UP:
            world.beasts[i].x--;
            break;
        case DOWN:
            world.beasts[i].x++;
            break;
        default:
            break;
    }

}
void validbeast()
{
    int x, y;
    for (int i=0; i<MAXBEASTS; i++)
    {
        if (world.beasts[i].status)
        {
            if (world.beasts[i].bushwait==0) {
                switch (world.beasts[i].direction) {
                    case RIGHT:
                        x = world.beasts[i].x;
                        y = world.beasts[i].y + 1;
                        movebestia(i, x, y, RIGHT);
                        break;
                    case LEFT:
                        x = world.beasts[i].x;
                        y = world.beasts[i].y - 1;
                        movebestia(i, x, y, LEFT);
                        break;
                    case UP:
                        x = world.beasts[i].x - 1;
                        y = world.beasts[i].y;
                        movebestia(i, x, y, UP);
                        break;
                    case DOWN:
                        x = world.beasts[i].x + 1;
                        y = world.beasts[i].y;
                        movebestia(i, x, y, DOWN);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                world.beasts[i].bushwait=0;
            }
            world.beasts[i].direction = STAY;
        }
    }
}
void* beast(void* arg)
{
    srand(time(NULL));
    struct beast* b = (struct beast*)arg;
    int move;
    setbeastvision(b, &world);
    if (!scanbeastmap(b, &move))
    {
        move = rand()%5;
    }
    b->direction = move;

    return NULL;
}
void* server_gameloop(void* arg)
{
    while (server_status)
    {
        for (int i=0; i < world.beastamount; i++)
        {
            pthread_create(&world.beasts[i].beast_thread, NULL, beast, (void*)&world.beasts[i]);
        }
        for (int i=0; i < world.beastamount; i++)
        {
            pthread_join(world.beasts[i].beast_thread, NULL);
        }
        clear();
        countround();
        validbeast();
        validmoves();
        for (int i = 0; i < MAXPLAYERS; i++)
        {
            if (world.players[i].status)
            {
                setplayervision(&world.players[i], &world);
                send(world.players[i].socket, (void*) &world.players[i], sizeof(world.players[i]), 0);
            }
        }

        printmap(&world);
        printlegend(world);
        serverprintplayerstats();
        refresh();
        pthread_mutex_unlock(&lock);
        usleep(200000);
    }
    return NULL;
}
void countround()
{
    for (int i=0; i < MAXPLAYERS; i++)
    {
        world.players[i].roundcounter++;
    }
    world.roundcounter++;
}
void playerscrash(struct player* p1, struct player* p2)
{
    int drop=0;
    drop += p1->carried_money;

    int x1 = p1->x;
    int y1 = p1->y;
    int x2 = p2->x;
    int y2 = p2->y;
    int val;
    if (p1->isbush)
    {
        val = 'B';
    }
    else
    {
        val = ' ';
    }
    killplayer(p1);
    killplayer(p2);
    if (dropmap[x2][y2] < 0) {
        dropmap[x2][y2] -= drop;
    } else {
        dropmap[x2][y2] += drop;
    }
    world.map[x1][y1] = val;
}
void serverprintplayerstats()
{
    int x = 3;
    for (int i=0; i < MAXPLAYERS; i++)
    {
        if (world.players[i].status)
        {
            mvprintw(MAPHEIGHT+2, x, "Player %c - ", (world.players[i].name + '0'));
            attron(COLOR_PAIR(ON_PAIR));
            mvprintw(MAPHEIGHT+2, x+12, "ON");
            attroff(COLOR_PAIR(ON_PAIR));
            mvprintw(MAPHEIGHT+3, x, "Socket: %d", world.players[i].socket);
            mvprintw(MAPHEIGHT+4, x, "Carried coins: %d", world.players[i].carried_money);
            mvprintw(MAPHEIGHT+5, x, "Balance: %d", world.players[i].balance);
            mvprintw(MAPHEIGHT+6, x, "Deaths: %d", world.players[i].deaths);
            mvprintw(MAPHEIGHT+7, x, "Current [X Y]: [%d %d]", world.players[i].y, world.players[i].x);
        }
        else
        {
            mvprintw(MAPHEIGHT+2, x, "Player %c - ", (world.players[i].name + '0'));
            attron(COLOR_PAIR(OFF_PAIR));
            mvprintw(MAPHEIGHT+2, x+12, "OFF");
            attroff(COLOR_PAIR(OFF_PAIR));
            mvprintw(MAPHEIGHT+3, x, "Socket: -");
            mvprintw(MAPHEIGHT+4, x, "Carried coins: -");
            mvprintw(MAPHEIGHT+5, x, "Balance: -");
            mvprintw(MAPHEIGHT+6, x, "Deaths: -");
            mvprintw(MAPHEIGHT+7, x, "Current [X Y]: [- -]");
        }
        x+=24;
    }
}
void disconnectplayer(struct player* p1)
{
    pthread_cancel(p1->player_thread);
    if (p1->isbush)
    {
        world.map[p1->x][p1->y]='B';
    }
    else
    {
        world.map[p1->x][p1->y] = ' ';
    }
    p1->x = p1->start_x;
    p1->y = p1->start_y;
    p1->status=0;
    p1->carried_money = 0;
    p1->campsite_x=-1;
    p1->campsite_y=-1;
    p1->deaths=0;
    p1->bushwait=0;
    p1->isbush=0;
}
void killplayer(struct player* p1)
{
    p1->deaths++;
    world.map[p1->x][p1->y] = 'D';
    if (p1->isbush)
    {
        dropmap[p1->x][p1->y] = -p1->carried_money;
    }
    else
    {
        dropmap[p1->x][p1->y] = p1->carried_money;
        if (p1->carried_money==0) world.map[p1->x][p1->y] = ' ';
    }

    p1->x = p1->start_x;
    p1->y = p1->start_y;
    world.map[p1->x][p1->y] = p1->name + '0';
    p1->carried_money = 0;
    p1->bushwait=0;
    p1->isbush=0;
}
void* client_move_handler(void* arg)
{
    struct player *p1 = (struct player *)arg;
    int key;
    while (server_status)
    {
        long res = recv(p1->socket, (void*) &key, sizeof(key), 0);
        if (!res)
        {
            disconnectplayer(p1);
            break;
        }
        switch (key) {
            case KEY_RIGHT:
                p1->direction=RIGHT;
                break;
            case KEY_LEFT:
                p1->direction=LEFT;
                break;
            case KEY_UP:
                p1->direction=UP;
                break;
            case KEY_DOWN:
                p1->direction=DOWN;
                break;
            default:
                break;
        }
    }
    return NULL;
}
void movegracz(int i, int x, int y, int move)
{
    int check=0;
    if (world.map[x][y] == ' ') {
        if (world.players[i].isbush) {
            world.map[world.players[i].x][world.players[i].y] = 'B';
        } else {
            world.map[world.players[i].x][world.players[i].y] = ' ';
        }
        world.players[i].isbush = 0;
        world.players[i].bushwait = 0;
        world.map[x][y] = world.players[i].name + '0';
        check=1;
    } else if (world.map[x][y] == 'B') {
        if (world.players[i].isbush) {
            world.map[world.players[i].x][world.players[i].y] = 'B';
        } else {
            world.map[world.players[i].x][world.players[i].y] = ' ';
        }
        world.players[i].isbush = 1;
        world.players[i].bushwait = 1;
        world.map[x][y] = world.players[i].name + '0';
        check=1;
    } else if (iscoin(world.map[x][y])) {
        if (world.players[i].isbush) {
            world.map[world.players[i].x][world.players[i].y] = 'B';
        } else {
            world.map[world.players[i].x][world.players[i].y] = ' ';
        }
        world.players[i].isbush = 0;
        world.players[i].bushwait = 0;
        world.players[i].carried_money += getcoin(world.map[x][y]);
        world.all_carried_money += getcoin(world.map[x][y]);
        world.map[x][y] = world.players[i].name + '0';
        check=1;
    } else if (world.map[x][y] == 'D') {
        if (world.players[i].isbush) {
            world.map[world.players[i].x][world.players[i].y] = 'B';
        } else {
            world.map[world.players[i].x][world.players[i].y] = ' ';
        }
        if (dropmap[x][y] < 0) {
            world.players[i].carried_money += (-dropmap[x][y]);
            world.all_carried_money += (-dropmap[x][y]);
            world.players[i].isbush = 1;
            world.players[i].bushwait = 1;
        } else {
            world.players[i].carried_money += dropmap[x][y];
            world.all_carried_money += dropmap[x][y];
            world.players[i].isbush = 0;
            world.players[i].bushwait = 0;
        }
        world.map[x][y] = world.players[i].name + '0';
        check=1;
    } else if (world.map[x][y] == 'A') {
        world.players[i].balance += world.players[i].carried_money;
        world.all_balance += world.players[i].carried_money;
        world.all_carried_money -= world.players[i].carried_money;
        world.players[i].carried_money = 0;
    } else if (isplayer(world.map[x][y])) {
        int playerindex = isplayer(world.map[x][y]) - 1;
        world.all_carried_money -= world.players[i].carried_money;
        world.all_carried_money -= world.players[playerindex].carried_money;
/*        killplayer(&world.players[i]);
        killplayer(&world.players[playerindex]);*/
        playerscrash(&world.players[i], &world.players[playerindex]);
    }

    if (check)
    {
        switch (move) {
            case RIGHT:
                world.players[i].y++;
                break;
            case LEFT:
                world.players[i].y--;
                break;
            case UP:
                world.players[i].x--;
                break;
            case DOWN:
                world.players[i].x++;
                break;
            default:
                break;
        }
    }
}
void validmoves()
{
    int x, y;

    for (int i=0; i<MAXPLAYERS; i++)
    {
        if (world.players[i].status)
        {
            if (world.players[i].bushwait==0) {
                switch (world.players[i].direction) {
                    case RIGHT:
                        x = world.players[i].x;
                        y = world.players[i].y + 1;
                        movegracz(i, x, y, RIGHT);
                        break;
                    case LEFT:
                        x = world.players[i].x;
                        y = world.players[i].y - 1;
                        movegracz(i, x, y, LEFT);
                        break;
                    case UP:
                        x = world.players[i].x - 1;
                        y = world.players[i].y;
                        movegracz(i, x, y, UP);
                        break;
                    case DOWN:
                        x = world.players[i].x + 1;
                        y = world.players[i].y;
                        movegracz(i, x, y, DOWN);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                world.players[i].bushwait=0;
            }
            world.players[i].direction = STAY;
        }
    }

}

int main()
{

    socket_server_start();

    worldinit(&world);
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(FALSE);
    start_color();
    init_pair(EMPTY_PAIR, COLOR_WHITE, COLOR_WHITE);
    init_pair(WALL_PAIR, COLOR_GREEN, COLOR_GREEN);
    init_pair(COIN_PAIR, COLOR_WHITE, COLOR_YELLOW);
    init_pair(BUSH_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(FOG_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(CAMP_PAIR, COLOR_BLACK, COLOR_CYAN);
    init_pair(PLAYER_PAIR, COLOR_WHITE, COLOR_BLUE);
    init_pair(BEAST_PAIR, COLOR_BLACK, COLOR_RED);
    init_pair(DROP_PAIR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(ON_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(OFF_PAIR, COLOR_BLACK, COLOR_RED);


    initplayerstructs(&world);

    pthread_create(&serverkeyboard, NULL, server_keyboard, NULL);
    pthread_create(&serverloop, NULL, server_gameloop, NULL);
    pthread_create(&clientinitializator, NULL, clientinit, NULL);

    pthread_join(serverloop, NULL);

    system("clear");
    printf("\nGame closed!\n\n");
    return 0;
}
