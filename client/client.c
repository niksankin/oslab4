#include "common.h"

void socket_close(int * fd)
{
    if (*fd != -1)
        close(*fd);
}

uint64_t ipc_sem_get_info(struct sem_data ** semds)
{
    uint64_t i = 0, maxid;

    struct sem_data * p;

    struct seminfo dummy;
    union semun arg;

    p = *semds = calloc(1, sizeof(struct sem_data));
    p->next = NULL;

    arg.array = (unsigned short *)(void *)&dummy;
    // maybe need error check here
    maxid = semctl(0, 0, SEM_INFO, arg);

    if (maxid == -1)
    {
        perror("semctl SEM_INFO error");
        free(*semds);
        return -1;
    }

    for (int j = 0; j <= maxid; j++)
    {
        int semid;
        struct semid_ds semseg;
        // struct ipc_perm *ipcp = &semseg.sem_perm;
        arg.buf = (struct semid_ds *)&semseg;

        semid = semctl(j, 0, SEM_STAT, arg);
        if (semid < 0)
        {
            continue;
        }

        p->semseg = semseg;
        p->next = calloc(1, sizeof(struct sem_data));
        p = p->next;
        p->next = NULL;
        i++;
    }

    if (i == 0)
        free(*semds);

    return i;
}

struct sem_data_serialized * ipc_sem_serialize(struct sem_data * semds,
                                               uint64_t size)
{
    struct sem_data_serialized * sem_arr =
        calloc(size, sizeof(struct sem_data_serialized));
    int i = 0;
    struct passwd * user_data;

    while (semds)
    {
        sem_arr[i].sem_nsems = (uint64_t)semds->semseg.sem_nsems;
        sem_arr[i].key = (uint64_t)semds->semseg.sem_perm.__key;
        sem_arr[i].uid = (uint64_t)semds->semseg.sem_perm.uid;
        sem_arr[i].cuid = (uint64_t)semds->semseg.sem_perm.cuid;

        user_data = getpwuid(sem_arr[i].uid);
        strcpy(sem_arr[i].user_name, user_data->pw_name);

        semds = semds->next;
        i++;
    }

    return sem_arr;
}

void ipc_sem_free_info(struct sem_data ** semds)
{
    while (*semds)
    {
        struct sem_data * next = (*semds)->next;
        free(*semds);
        *semds = next;
    }
}

void ipc_sem_serialize_free(struct sem_data_serialized ** sem_arr)
{
    free(*sem_arr);
}

int main(int argc, char * argv[])
{
    struct sockaddr_in serv_addr;
    int fd ON_SCOPE_EXIT(socket_close);

    uint64_t sem_size;

    struct sem_data * sem_data ON_SCOPE_EXIT(ipc_sem_free_info) = NULL;
    struct sem_data_serialized * sem_serialized ON_SCOPE_EXIT(
        ipc_sem_serialize_free) = NULL;

    uint32_t ping = 0xdeadbeef;
    uint32_t pong;

    char username[32];

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1337);
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    uint64_t r;

    if (argc != 2)
    {
        printf("Usage ./client USERNAME\n");
        return 0;
    }

    strcpy(username, argv[1]);

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket error");
        return -1;
    }

    sem_size = ipc_sem_get_info(&sem_data);

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("socket error");
        return -1;
    }

    do
    {
        send(fd, (const char *)&ping, sizeof(ping), 0);
        sleep(1);
        r = recv(fd, (char *)&pong, sizeof(pong), MSG_WAITALL);
    } while (r == -1);

    if (send(fd, (const char *)&sem_size, sizeof(sem_size), 0) == -1)
    {
        perror("socket error");
        return -1;
    }

    if (!sem_size)
        return 0;

    sem_serialized = ipc_sem_serialize(sem_data, sem_size);

    if (send(fd, (const char *)sem_serialized,
             sem_size * sizeof(struct sem_data_serialized), 0) == -1)
    {
        perror("socket error");
        return -1;
    }

    if (send(fd, username, sizeof(username), 0) == -1)
    {
        perror("socket error");
        return -1;
    }

    return 0;
}