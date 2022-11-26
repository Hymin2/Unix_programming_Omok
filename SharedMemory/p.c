#include "pv.h"

int p(int semid){
	struct sembuf p_buf;

	p_buf.sem_num = 0;
	p_buf.sem_op = -1;
	p_buf.sem_flg = SEM_UNDO;

	if(semop(semid, &p_buf, 1) == -1){
		perror("p(semid) failed");
		exit(1);
	}

	return 0;
}
