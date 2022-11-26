#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdlib.h>

#define SEMPERM 0600
#define TRUE 1
#define FALSE 0

typedef union semun{
	int val;
	struct semid_ds *buf;
	ushort *array;
} semun;
