#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1111
#define MAXDATASIZE 100

char sendbuf[1024];
char recvbuf[1024];
char name[100];
int fd;
int board[9] = {'/', '/', '/', '/', '/', '/', '/', '/', '/'};

//it means this person is inviter(1) or guest(2)
int side = 0;

void usage()
{
    printf("\"show\" to show all user\n");
    printf("\"invite (username)\" to invite other player\n");
    printf("\"accept/decline (username)\" to accept or decline an invitation\n");
    printf("\"logout\" to logout\n");
}

void print_board(int *board)
{
    printf(" 0 │ 1 │ 2          %c │ %c │ %c \n", board[0], board[1], board[2]);
    printf("───┼───┼───        ───┼───┼───\n");
    printf(" 3 │ 4 │ 5          %c │ %c │ %c \n", board[3], board[4], board[5]);
    printf("───┼───┼───        ───┼───┼───\n");
    printf(" 6 │ 7 │ 8          %c │ %c │ %c \n", board[6], board[7], board[8]);
}

int choose_user_turn(int *board)
{
    if (side == 1)
        return 'O';
    else
        return 'X';
}

// modify chess board, and fill "sendbuf" with package format.
void write_on_board(int *board, int location)
{
    //print_board(board);
    int user_choice = choose_user_turn(board);
    // Record which location on board is selected by inviter.
    board[location] = user_choice;
    sprintf(sendbuf, "start %d %d %d %d %d %d %d %d %d\n", board[0],
            board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8]);
}

// Only handle message from server to client.
void pthread_recv(void *ptr)
{
    int instruction;
    while (1)
    {
        memset(sendbuf, 0, sizeof(sendbuf));
        instruction = 0;
        // recvbuf is filled by server's fd.
        if ((recv(fd, recvbuf, MAXDATASIZE, 0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }
        sscanf(recvbuf, "%d", &instruction);
        switch (instruction)
        {

        case 0:
        {
            printf("%s", &recvbuf[2]);
            break;
        }
        case 1:
        {
            printf("%s", &recvbuf[2]);
            break;
        }
        case 2:
        {
            char inviter[100];
            printf("active\n");
            printf("%s\n", &recvbuf[2]);
            break;
        }
        case 3:
        {
            printf("Game Start!\n");
            printf("/ is empty space\n");
            printf("Please input:-0~8\n");

            if (recvbuf[2] == '1')
                side = 1;
            else
                side = 2;

            print_board(board);
            break;
        }
        case 4:
        {
            // int board[9];
            char msg[100];
            sscanf(recvbuf, "%d %d%d%d%d%d%d%d%d%d %s", &instruction, &board[0], &board[1], &board[2], &board[3],
                   &board[4], &board[5], &board[6], &board[7], &board[8], msg);

            print_board(board);
            printf("%s\n", msg);
            printf("Please input:-0~8\n");
            break;
        }

        default:
            break;
        }

        memset(recvbuf, 0, sizeof(recvbuf));
    }
}

int main(int argc, char *argv[])
{
    int numbytes;
    char buf[MAXDATASIZE];
    char port[100] = "0.0.0.0";
    struct hostent *he;
    struct sockaddr_in server;

    if ((he = gethostbyname(port)) == NULL)
    {
        printf("gethostbyname() error\n");
        exit(1);
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        exit(1);
    }

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    if (connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        printf("connect() error\n");
        exit(1);
    }

    // First, Add User.
    printf("connect success\n");
    char str[] = " have come in\n";
    printf("Please login：");
    fgets(name, sizeof(name), stdin);
    name[strlen(name) - 1] = 0;

    char package[100] = {0};
    strcat(package, "1 ");
    strcat(package, name);
    send(fd, name, (strlen(package)), 0);

    // usage
    usage();

    // Only handle message from server to client. (Goto pthread_recv finction)
    pthread_t tid;
    pthread_create(&tid, NULL, (void *)pthread_recv, NULL);

    // Only handle message from client to server.
    while (1)
    {
        memset(sendbuf, 0, sizeof(sendbuf));
        fgets(sendbuf, sizeof(sendbuf), stdin);
        sendbuf[strlen(sendbuf) - 1] = 0;

        int location;
        if (isdigit(sendbuf[0]))
        {
            sscanf(&sendbuf[0], "%d", &location);
            write_on_board(board, location);
        }
        send(fd, sendbuf, strlen(sendbuf), 0); // Send instructions to server

        // Logout
        if (!strcmp(sendbuf, "logout"))
        {
            memset(sendbuf, 0, sizeof(sendbuf));
            printf("You have Quit.\n");
            return 0;
        }
    }

    pthread_join(tid, NULL);
    close(fd);
    return 0;
}
