#include "sharemem.h"
#include "pv.h"
#include "p.c"
#include "v.c"
#include "initsem.c"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define SHMKEY1 (key_t) 60100
#define SHMKEY2 (key_t) 60101
#define SEMKEY1 1111
#define SEMKEY2 2222

int shmid1, shmid2;
int semid1, semid2;

data_buf* shared_mem1;
data_buf* shared_mem2;

void initSharedMemoryData();
void* watingRoomDataCommunication();
void* gameRoomDataCommunication();
void getseg();
void* judgeOmok(void* turn);

int main(){
	pthread_t waiting_thread;
	pthread_t game_thread;
	
	getseg();
	
	semid1 = initsem(SEMKEY1);
	semid2 = initsem(SEMKEY2);

	initSharedMemoryData();
	
	pthread_create(&waiting_thread, NULL, watingRoomDataCommunication, NULL);
	pthread_join(waiting_thread, NULL);

	pthread_create(&game_thread, NULL, gameRoomDataCommunication, NULL);
        pthread_join(game_thread, NULL);

	sleep(5);

	if(shmctl(shmid1, IPC_RMID, 0) == -1){
		printf("shmctl error\n");
	}
	if(shmctl(shmid2, IPC_RMID, 0) == -1){
                printf("shmctl error\n");
        }
	if(semctl(semid1, IPC_RMID, 0) == -1){
                printf("semctl error\n");
        }
	if(semctl(semid2, IPC_RMID, 0) == -1){
                printf("semctl error\n");
        }
}

void getseg(){
        if((shmid1 = shmget(SHMKEY1, sizeof(data_buf), 0666 | IFLAGS)) == -1)
		printf("shmget error");

        if((shared_mem1 = (data_buf *) shmat(shmid1, 0, 0)) == ERRO)
                printf("shmget error");

	if((shmid2 = shmget(SHMKEY2, sizeof(data_buf), 0666 | IFLAGS)) == -1)
                printf("shmat error");

        if((shared_mem2 = (data_buf *) shmat(shmid2, 0, 0)) == ERRO)
                printf("shmat error");
}

void initSharedMemoryData(){
	p(semid1);
	shared_mem1->wait_msg.connect = 0;
	shared_mem1->wait_msg.ready = 0;
	shared_mem1->wait_msg.status_change = 0;
	shared_mem1->wait_msg.opponent_connect = 0;
	shared_mem1->wait_msg.opponent_ready = 0;
	shared_mem1->wait_msg.opponent_change = 0;

	shared_mem1->game_msg.my_turn = 0;
	shared_mem1->game_msg.result = 0;
	shared_mem1->game_msg.row = 0;
	shared_mem1->game_msg.col = 0;
	shared_mem1->game_msg.turn_end = 0;

	p(semid2);
	shared_mem2->wait_msg.connect = 0;
        shared_mem2->wait_msg.ready = 0;
        shared_mem2->wait_msg.status_change = 0;
        shared_mem2->wait_msg.opponent_connect = 0;
        shared_mem2->wait_msg.opponent_ready = 0;
        shared_mem2->wait_msg.opponent_change = 0;

	shared_mem2->game_msg.my_turn = 0;
        shared_mem2->game_msg.result = 0;
	shared_mem2->game_msg.row = 0;
        shared_mem2->game_msg.col = 0;
        shared_mem2->game_msg.turn_end = 0;

	for(int i = 0; i < ROW; i++){
		for(int k = 0; k < COLUMN; k++){
			shared_mem1->game_msg.omok_board[i][k] = '+';
			shared_mem2->game_msg.omok_board[i][k] = '+';
		}
	}
	v(semid1);
	v(semid2);
}

void* gameRoomDataCommunication(){
	void* ret;
	int turn;
	double accum;
	pthread_t judge_thread;

	p(semid1);
        shared_mem1->game_msg.my_turn = 1;
        v(semid1);

	while(1){
	
		while(1){
			if(shared_mem1->game_msg.turn_end == 1){
				clock_gettime(CLOCK_MONOTONIC, &shared_mem1->stop);
	                        accum = (shared_mem1->stop.tv_sec - shared_mem1->start.tv_sec) + (double)(shared_mem1->stop.tv_nsec - shared_mem1->start.tv_nsec) / (double)BILLION;
        	                printf("%.9f\n",accum);

				p(semid2);
				shared_mem2->game_msg.omok_board[shared_mem1->game_msg.row][shared_mem1->game_msg.col] = 'X';
				shared_mem2->game_msg.my_turn = 1;
				v(semid2);

				p(semid1);
				shared_mem1->game_msg.my_turn = 0;
				shared_mem1->game_msg.turn_end = 0;
				v(semid1);
				
				turn = 1;

				pthread_create(&judge_thread, NULL, judgeOmok, (void*)&turn);
				pthread_join(judge_thread,(void*) &ret);

				break;
			}
		}

		if(*(int*)ret == 1){
			p(semid1);
			shared_mem1->game_msg.result = 1;
			v(semid1);

			p(semid2);
                        shared_mem2->game_msg.result = 2;
                        v(semid2);

			break;	
		}

		while(1){
                        if(shared_mem2->game_msg.turn_end == 1){
                 		clock_gettime(CLOCK_MONOTONIC, &shared_mem2->stop);
        	                accum = (shared_mem2->stop.tv_sec - shared_mem2->start.tv_sec) + (double)(shared_mem2->stop.tv_nsec - shared_mem2->start.tv_nsec) / (double)BILLION;
	                        printf("%.9f\n",accum);

		 		p(semid1);
                                shared_mem1->game_msg.omok_board[shared_mem2->game_msg.row][shared_mem2->game_msg.col] = 'X';
                 		shared_mem1->game_msg.my_turn = 1;
		 		v(semid1);

                                p(semid2);
                                shared_mem2->game_msg.my_turn = 0;
                                shared_mem2->game_msg.turn_end = 0;
                                v(semid2);

				turn = 2;

				pthread_create(&judge_thread, NULL, judgeOmok, (void*)&turn);
                                pthread_join(judge_thread,(void*) &ret);
                                break;
                        }
                }

                if(*(int*)ret == 1){
                        p(semid2);
                        shared_mem2->game_msg.result = 1;
                        v(semid2);

                        p(semid1);
                        shared_mem1->game_msg.result = 2;
                        v(semid1);

			break;
                }

	}	
}

void* judgeOmok(void* turn){
	int i, k, cnt = 0, t = *((int*)turn);
	int row, col, ret = 0;
	char board[ROW][COLUMN];

	if(t == 1){
		for(i = 0; i < ROW; i++){
			for(k = 0; k < COLUMN; k++){
				board[i][k] = shared_mem1->game_msg.omok_board[i][k];
			}
		} 
		row = shared_mem1->game_msg.row;
		col = shared_mem1->game_msg.col;
	}

	else{
		for(i = 0; i < ROW; i++){
                        for(k = 0; k < COLUMN; k++){
                                board[i][k] = shared_mem2->game_msg.omok_board[i][k];
                        }
                }
                row = shared_mem2->game_msg.row;
                col = shared_mem2->game_msg.col;

	}


	for(i = 0; i < ROW; i++){
		if(board[i][col] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
			if(board[i + 1][col] != 'O'){
				ret = 1;
			        pthread_exit((void*)&ret);	
			}
		}	
	}

	cnt = 0;
	for(i = 0; i < COLUMN; i++){
                if(board[row][i] == 'O') cnt++;
                else cnt = 0;

		if(cnt == 5){
                         if(board[row][i + 1] != 'O'){
                                 ret = 1;
                                 pthread_exit((void*)&ret);
                         }
                 }

        }


	int start_row, start_col;
	if(row >= col){
		cnt = 0;
		start_row = row - col;
		start_col = 0;
		for(i = 0; start_row + i < 15; i++){
			if(board[start_row + i][start_col + i] == 'O') cnt++;
                	else cnt = 0;

			if(cnt == 5){
                          if(board[i][col + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

		}
        }
	else{
		cnt = 0;
                start_row = 0;
                start_col = col - row;
                for(i = 0; start_col + i < 15; i++){
                        if(board[start_row + i][start_col + i] == 'O') cnt++;
                        else cnt = 0;

			if(start_col < 15 && cnt == 5){
                          if(board[start_row + i + 1][start_col + i] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

                }
	
	}

	start_row = row + col;
	start_col = 0;
	cnt = 0;
	for(i = 0; start_row - i < 0; i++){
		if(board[start_row - i][i] == 'O') cnt++;
		else cnt = 0;

		if(start_row - i - 1 >= 0 && cnt == 5){
                          if(board[start_row - i - 1][i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

	}

	ret = 0;
	printf("%d",ret);
	pthread_exit((void*)&ret);
	
}

void* watingRoomDataCommunication(){
	int player1_ready = 0;
	int player2_ready = 0;
	double accum;
	while(player1_ready == 0 || player2_ready == 0){
		if(shared_mem1->wait_msg.status_change == 1){
			clock_gettime(CLOCK_MONOTONIC, &shared_mem1->stop);
			accum = (shared_mem1->stop.tv_sec - shared_mem1->start.tv_sec) + (double)(shared_mem1->stop.tv_nsec - shared_mem1->start.tv_nsec) / (double)BILLION;
			printf("%.9f\n",accum);

			if(shared_mem1->wait_msg.connect == 1){
				p(semid2);
				shared_mem2->wait_msg.opponent_change = 1;
				shared_mem2->wait_msg.opponent_connect = 1;
				v(semid2);
			}
			else if(shared_mem1->wait_msg.connect == 0){
                      		p(semid2);
				shared_mem2->wait_msg.opponent_change = 1;
                   		shared_mem2->wait_msg.opponent_connect = 0;
                      		v(semid2);
              		}

			if(player1_ready == 0 && shared_mem1->wait_msg.ready == 1){
                       		p(semid2);
				shared_mem2->wait_msg.opponent_change = 1;
                       		shared_mem2->wait_msg.opponent_ready = 1;
                        	v(semid2);

                        	player1_ready = 1;
                	}
			p(semid1);
			shared_mem1->wait_msg.status_change = 0;
			v(semid1);
		}

		if(shared_mem2->wait_msg.status_change == 1){
			clock_gettime(CLOCK_MONOTONIC, &shared_mem2->stop);
                        accum = (shared_mem2->stop.tv_sec - shared_mem2->start.tv_sec) + (double)(shared_mem2->stop.tv_nsec - shared_mem2->start.tv_nsec) / (double)BILLION;
                        printf("%.9f\n",accum);

			if(shared_mem2->wait_msg.connect == 1){
                        	p(semid1);
				shared_mem1->wait_msg.opponent_change = 1;
                        	shared_mem1->wait_msg.opponent_connect = 1;
                        	v(semid1);
                	}
			else if(shared_mem2->wait_msg.connect == 0){
                        	p(semid1);
				shared_mem1->wait_msg.opponent_change = 1;
                        	shared_mem1->wait_msg.opponent_connect = 0;
                        	v(semid1);
                	}

			if(player2_ready == 0 && shared_mem2->wait_msg.ready == 1){
                        	p(semid1);
				shared_mem1->wait_msg.opponent_change = 1;
                       		shared_mem1->wait_msg.opponent_ready = 1;
                        	v(semid1);

				player2_ready = 1;
                	}
			p(semid2);
			shared_mem2->wait_msg.status_change = 0;
			v(semid2);
		}
	}

	pthread_exit(NULL);
}


