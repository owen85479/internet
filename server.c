#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>

#define PORT 1111
#define BACKLOG 1
#define Max 5
#define MAXSIZE 1024

struct userinfo
{
    char id[100], password[100];
    int playwith, online, no;
    int fd;
};

struct userinfo users[100];
int user_num;
int fdt[Max] = {0};
int logout = 0;
char mes[1024];
int win_dis[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

int find_no(char *name)
{
    int i;
    for (i = 0; i < 100; i++)
        if (strcmp(name, users[i].id) == 0)
            return i;
    return -1;
}

void message_handler(char *mes, int sender, struct userinfo *current_user)
{
    int i;
    int instruction = 0;
    char temp[100];
    strcpy(temp, mes);
    char *instruct = strtok(temp, " ");

    if (!strcmp(instruct, "logout"))
    {
        current_user->online = 0;
        logout = 1;
    }
    else if (!strcmp(instruct, "show"))
    {
        char buf[MAXSIZE] = "1 ";

        for (i = 0; i < 100; i++)
            if (users[i].online != 0)
            {
                strcat(buf, users[i].id);
                strcat(buf, "\n");
            }
        printf("%s\n", buf);
        send(current_user->fd, buf, strlen(buf), 0);
    }
    else if (!strcmp(instruct, "invite"))
    {
        char *guest_name, buf[MAXSIZE];

        guest_name = strtok(NULL, " ");
        int guest_no = find_no(guest_name);
        sprintf(buf, "2 %s invite you. Accept?\n", current_user->id);

        send(users[guest_no].fd, buf, strlen(buf), 0);
    }
    else if (!strcmp(instruct, "accept"))
    {
        char *inviter = strtok(NULL, " ");
        int inviter_no = find_no(inviter);

        users[inviter_no].playwith = current_user->no;
        current_user->playwith = inviter_no;

        send(current_user->fd, "3 2\n", 4, 0);
        send(users[inviter_no].fd, "3 1\n", 4, 0);
    }
    else
    {
        int board[9];
        char state[100] = {0};
        char buf[MAXSIZE] = {0};

        sscanf(mes, "start %d %d %d %d %d %d %d %d %d", &board[0], &board[1], &board[2], &board[3], &board[4], &board[5], &board[6], &board[7], &board[8]);

        for (i = 0; i < 8; i++)
        {
            if (board[win_dis[i][0]] == board[win_dis[i][1]] && board[win_dis[i][1]] == board[win_dis[i][2]])
            {
                if (board[win_dis[i][0]] != '/')
                {
                    sprintf(state, "%s_Win!\n", current_user->id);
                    sprintf(buf, "4 %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);

                    send(current_user->fd, buf, sizeof(buf), 0);
                    send(users[current_user->playwith].fd, buf, sizeof(buf), 0);
                    return;
                }
            }
        }

        for (i = 0; i < 9; i++)
        {
            if (i == 8)
            {
                strcat(state, "Tie!\n");
                sprintf(buf, "4 %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1], board[2], board[3], board[4],
                        board[5], board[6], board[7], board[8], state);

                send(current_user->fd, buf, sizeof(buf), 0);
                send(users[current_user->playwith].fd, buf, sizeof(buf), 0);
                return;
            }
            if (board[i] == '/')
                break;
        }

        sprintf(state, "it's_%s's_turn\n", users[current_user->playwith].id);
        printf("state=%s", state);
        sprintf(buf, "4 %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1], board[2], board[3], board[4], board[5], board[6], board[7], board[8], state);
        printf("buf=%s", buf);

        send(current_user->fd, buf, sizeof(buf), 0);
        send(users[current_user->playwith].fd, buf, sizeof(buf), 0);
    }
}

void *pthread_service(void *sfd)
{
    int fd = *(int *)sfd;
    int i;
    struct userinfo *current_user;
    char name[100] = {0}, password[100] = {0};
    int success = 0;

    while (success == 0)
    {
        recv(fd, mes, MAXSIZE, 0);
        strcpy(name, strtok(mes, " "));
        strcpy(password, strtok(NULL, " "));

        bzero(mes, MAXSIZE);

        for (i = 0; i < user_num; i++)
            if (!strcmp(users[i].id, name))
                if (users[i].online == 1)
                {
                    send(fd, "0 the user have logged in!\n", 28, 0);
                    printf("error!\n");
                    break;
                }
                else if (strcmp(users[i].password, password))
                {
                    send(fd, "0 password error!\n", 19, 0);
                    printf("error!\n");
                    break;
                }
                else
                {
                    current_user = &users[i];
                    current_user->online = 1;
                    current_user->fd = fd;

                    success = 1;
                    send(fd, "0 welcome back!\n", 17, 0);
                    printf("user %d online\n", i);
                    break;
                }
    }

    while (1)
    {
        memset(mes, 0, MAXSIZE);
        recv(fd, mes, MAXSIZE, 0);
        printf("%s\n", mes);

        message_handler(mes, fd, current_user);
        bzero(mes, MAXSIZE);

        if (logout == 1)
            break;
    }
    close(fd);
}

int main()
{
    int i, j;
    int listenfd, connectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sin_size;
    sin_size = sizeof(struct sockaddr_in);
    int number = 0;
    int fd;
    FILE *user_info = fopen("./user_info.txt", "r");

    for (i = 0; i < 100; i++)
    {
        char temp[200];
        if (fgets(temp, 200, user_info) == NULL)
        {
            user_num = i;
            break;
        }

        strcpy(users[i].id, strtok(temp, " "));
        strcpy(users[i].password, strtok(NULL, " "));
        users[i].password[strlen(users[i].password) - 1] = 0;
        users[i].playwith = -1;
        users[i].no = i;
        users[i].online = 0;

        printf("%s %s\n", users[i].id, users[i].password);
    }

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Creating socket failed.");
        exit(1);
    }

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind error.");
        exit(1);
    }

    if (listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error\n");
        exit(1);
    }
    printf("Waiting for client....\n");

    while (1)
    {
        if ((fd = accept(listenfd, (struct sockaddr *)&client, &sin_size)) == -1)
        {
            perror("accept() error\n");
            exit(1);
        }

        if (number >= Max)
        {
            printf("no more client is allowed\n");
            close(fd);
        }

        for (i = 0; i < Max; i++)
        {
            if (fdt[i] == 0)
            {
                fdt[i] = fd;
                break;
            }
        }
        pthread_t tid;
        pthread_create(&tid, NULL, (void *)pthread_service, &fd);
        number = number + 1;
    }
    close(listenfd);
}
