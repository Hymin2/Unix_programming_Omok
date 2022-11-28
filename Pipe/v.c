#include "pv.h"

int v(int semid){
	struct sembuf v_buf;

	v_buf.sem_num = 0;
	v_buf.sem_op = 1;
	v_buf.sem_flg = SEM_UNDO;

	if(semop(semid, &v_buf, 1) == -1){
		perror("v (semid) failed");
		exit(1);
	}
	return 0;
}
