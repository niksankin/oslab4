#include "common.h"
#include <errno.h>
extern int errno;
void socket_close(int *fd)
{
	if (*fd != -1)
		close(*fd);
}

void ipc_sem_serialize_free(struct sem_data_serialized** sem_arr)
{
	free(*sem_arr);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in serv_addr, cli_addr;
	int cli_len = sizeof(cli_addr);
	int fd ON_SCOPE_EXIT(socket_close);
	struct sem_data_serialized* sem_arr ON_SCOPE_EXIT(ipc_sem_serialize_free);
	uint64_t arr_size;
	uint32_t pong;

	char username[32];

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(1337);
	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) //creating socket
	{
		perror("socket error");
		return -1;
	}

	if (bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	{
		perror("bind error");
		return -1;
	}

	if (recvfrom(fd, (char *)&pong, sizeof(pong),
		MSG_WAITALL, (struct sockaddr *) &cli_addr,
		&cli_len) == -1)
	{
		perror("read error");
		return -1;
	}

	sendto(fd, (const char *)&pong, sizeof(pong),
		0, (const struct sockaddr *) &cli_addr,
		sizeof(cli_addr));
	
	if (recvfrom(fd, (char *)&arr_size, sizeof(arr_size),
		MSG_WAITALL, (struct sockaddr *) &cli_addr,
		&cli_len) == -1)
	{
		perror("read error");
		return -1;
	}

	if (!arr_size)
		return 0;

	sem_arr = calloc(arr_size, sizeof(struct sem_data_serialized));

	if (recvfrom(fd, (char *)sem_arr, sizeof(struct sem_data_serialized)*arr_size,
		MSG_WAITALL, (struct sockaddr *) &cli_addr,
		&cli_len) == -1)
	{
		perror("read error");
		return -1;
	}

	if (recvfrom(fd, username, sizeof(username),
		MSG_WAITALL, (struct sockaddr *) &cli_addr,
		&cli_len) == -1)
	{
		perror("read error");
		return -1;
	}

	printf("User owned semaphore arrays:\n");
	printf("===================================\n");
	printf("=  Key  =    Num of semaphores    =\n");
	printf("===================================\n");

	for (uint64_t i = 0; i < arr_size; ++i)
	{
		if (!strcmp(sem_arr[i].user_name, username))
			printf("=%5d  =%13d            =\n", sem_arr[i].key, sem_arr[i].sem_nsems);
	}

	printf("===================================\n");

	return 0;
}