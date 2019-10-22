#pragma once

#define _DEFAULT_SOURCE

#define ON_SCOPE_EXIT(destructor) __attribute__((__cleanup__(destructor)))

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#if !defined(__GNU_LIBRARY__) || defined(_SEM_SEMUN_UNDEFINED)
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                  /* value for SETVAL */
	struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;    /* array for GETALL, SETALL */
							  /* Linux specific part: */
	struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif


struct sem_data_serialized
{
	uint64_t key;
	uint64_t uid;
	uint64_t cuid;
	uint64_t sem_nsems;
	char user_name[32];
};

struct sem_data {
	struct semid_ds semseg;
	struct sem_data *next;
};