#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <pthread.h>
#include <netinet/in.h>
#include "game.h"
#define PORT 8080
int sock = 0, client_fd;
struct sockaddr_in serv_addr;
int cpu=0;

Player p1;
int player_status;
pthread_t playergameloop;
pthread_t playerkeyboard;

int generatemove()
{
    srand(time(NULL));
    int key = rand()%4;
    switch (key)
    {
        case 0:
            key = KEY_RIGHT;
            break;
        case 1:
            key = KEY_LEFT;
            break;
        case 2:
            key = KEY_UP;
            break;
        case 3:
            key = KEY_DOWN;
            break;
    }
    return key;
}
void socket_client_start()
{
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        printf(
                "\nInvalid address/ Address not supported \n");
        exit(-1);
    }

    if ((client_fd
                 = connect(sock, (struct sockaddr*)&serv_addr,
                           sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        exit(-1);
    }
    player_status=1;
}
void playerleave()
{
    player_status=0;
    endwin();
    close(client_fd);
    system("clear");
}
void* player_gameloop(void* arg)
{

    while(player_status)
    {
        long res = recv(sock, (void*) &p1, sizeof(p1), 0);
        if (!res)
        {
            playerleave();
            printf("\nServer closed!\n\n");
            pthread_cancel(playerkeyboard);
            pthread_exit(NULL);
        }
        clear();
        printplayermap(p1);
        printplayerstats(p1);
        printlegendplayer();
        refresh();
        if (cpu)
        {
            int key = generatemove();
            send(sock, (void*) &key, sizeof(key), 0);
        }
    }
    return NULL;
}
void* player_keyboard(void* arg)
{
    int key;
    while (player_status)
    {
        key = getch();
        if (key=='q' || key=='Q')
        {
            playerleave();
            pthread_cancel(playergameloop);
            pthread_exit(NULL);
        }
        if (cpu==0)
        {
            if (key==KEY_RIGHT || key==KEY_LEFT || key==KEY_UP || key==KEY_DOWN)
            {
                send(sock, (void*) &key, sizeof(key), 0);
            }
        }
    }
    return NULL;
}

int main()
{
/*    int ch;
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(FALSE);
    mvprintw(0, 0, "Choose player type (Human - H | Computer - C)");
    while (1)
    {
        ch = getch();
        if (ch=='c' || ch=='C')
        {
            cpu=1;
            break;
        }
        if (ch=='h' || ch=='H')
        {
            cpu=0;
            break;
        }
    }
    endwin();*/

    socket_client_start();

    int check;
    recv(sock, (void*) &check, sizeof(check), 0);
    if (!check)
    {
        printf("\nServer is full!\n\n");
        return 0;
    }

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

    pthread_create(&playerkeyboard, NULL, player_keyboard, NULL);
    pthread_create(&playergameloop, NULL, player_gameloop, NULL);


    pthread_join(playergameloop, NULL);


    printf("\nClient closed!\n\n");
    return 0;
}

