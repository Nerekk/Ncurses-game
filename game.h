#ifndef MAZEGAMEPROJECT_GAME_H
#define MAZEGAMEPROJECT_GAME_H

#include <pthread.h>
#define MAPHEIGHT 23
#define MAPWIDTH 46
#define MAXPLAYERS 4
#define MAXBEASTS 10

#define EMPTY_PAIR 1
#define WALL_PAIR 2
#define COIN_PAIR 3
#define BUSH_PAIR 4
#define PLAYER_PAIR 5
#define BEAST_PAIR 6
#define FOG_PAIR 7
#define CAMP_PAIR 8
#define DROP_PAIR 9
#define ON_PAIR 10
#define OFF_PAIR 11

enum moneytype{COIN, TREASURE, LARGE_TREASURE};
enum direction{LEFT=0, RIGHT, UP, DOWN, STAY};

typedef struct player {
    char tab[MAPHEIGHT][MAPWIDTH];
    int name;
    int direction;
    int x;
    int y;
    int start_x;
    int start_y;
    int isbush;
    int bushwait;
    int carried_money;
    int balance;
    int socket;
    int status;
    int roundcounter;
    int campsite_x;
    int campsite_y;
    int deaths;
    pthread_t player_thread;
} Player;

typedef struct beast {
    char tab[5][5];
    int name;
    int direction;
    int x;
    int y;
    int bushwait;
    int status;
    int laststep;
    pthread_t beast_thread;
} Beast;

typedef struct world{
    char **map;
    Player players[MAXPLAYERS];
    Beast beasts[MAXBEASTS];
    int beastamount;
    int roundcounter;
    int all_carried_money;
    int all_balance;
} World;

void worldinit(struct world* world);
void freemap(struct world* w);
void spawnmoney(struct world* world, enum moneytype type);
void printmap(struct world* world);
void printplayermap(struct player p);
void setplayervision(struct player* p, struct world* world);
void printlegend(struct world world);
void printlegendplayer();
void socket_server_start();
void free_coords(int* x, int* y, struct world* world);
void initplayerstructs(struct world* world);
int iscoin(char c);
int getcoin(char c);
int isplayer(char c);
void killplayer(struct player* p1);
void disconnectplayer(struct player* p1);
void printplayerstats(struct player p);
void countround();
void serverprintplayerstats();
void validmoves();
void movegracz(int i, int x, int y, int move);

void initbeast();
void validbeast();
void setbeastvision(struct beast* p, struct world* world);
int scanbeastmap(struct beast* p, int* move);
void movebestia(int i, int x, int y, int move);
void* client_move_handler(void* arg);
void* beast(void* arg);


#endif
