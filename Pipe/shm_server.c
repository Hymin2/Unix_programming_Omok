#include "sharemem.h"
#include "pv.h"
#include "p.c"
#include "v.c"
#include "initsem.c"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/msg.h>
#include <fcntl.h>

#define FROM_CLNT_FILE ".fifo/from_client"
#define TO_CLNT_FILE ".fifo/to_client_"
#define LOG_FILE "fifo_log.txt"
#define USER_ID_SIZE 21
#define BUF_SIZE 1024
#define SERVER 1
#define CONNECT 32768+1
#define PATH_SIZE 50

#define SHMKEY1 (key_t) 60100
#define SHMKEY2 (key_t) 60101
#define SEMKEY1 1111
#define SEMKEY2 2222

typedef struct msgtype{
	long mtype;
	char data[BUF_SIZE];
	long src;
}msg_t;

typedef struct Room{
	long clnt[2];
} Room;

int shmid1, shmid2;
int semid1, semid2;

data_buf* shared_mem1;
data_buf* shared_mem2;

void receiveConnectionMessage(Room* room);
void* writeLogToTextFile();

void initSharedMemoryData();
void* watingRoomDataCommunication();
void* gameRoomDataCommunication();
void getseg();
void* judgeOmok(void* turn);

int main(){
	pthread_t waiting_thread, game_thread;
	Room room;
	void* game_thread_return;
	int i, fd;

	pipe(log_fds);
	pthread_create(&waiting_thread, NULL, writeLogToTextFile, NULL);
	receiveConnectionMessage(&room);
	pthread_create(&game_thread, NULL, gameRoomDataCommunication, &room);
	pthread_join(waiting_thread, NULL);
	pthread_join(game_thread, &game_thread_return);

	return 0;
}

void receiveConnectionMessage(Room* room)
{
	int i;
	int read_fd, write_fd;
	msg_t msg;
	char path[PATH_SIZE];

	system("clear");

	mkfifo(FROM_CLNT_FILE, 0666);

	for (i=0; i<2; i++)
	{
		memset(msg.data, 0 , BUF_SIZE);

		if ((read_fd = open(FROM_CLNT_FILE, O_RDWR)) == -1)
		{
			perror("[SYSTEM] Open Error!!!\n");
		}
		if (read(read_fd, &msg, sizeof(msg)) == -1)
		{
			perror("[SYSTEM] Read Error!!!\n");
		}

		room->clnt[i] = msg.src;
		
		printf("[SYSTEM] player %d id : %ld\n", i+1, room->clnt[i]);
		sprintf(log_msg, "[SYSTEM] player%d id : %ld\n"i+1, room->clnt[i]);
		write(log_fds[1], log_msg, strlen(log_msg));

		sprintf(path, "%s%ld", TO_CLNT_FILE, room->clnt[i]);
		mkfifo(path, 06666);
	}
	sprintf(log_msg,"Complete\n");
	write(log_fds[1],log_msg,strlen(log_msg);
}

void* writeLogToTextFile()
{
	int len;
	FILE* fp = NULL;
	while (1)
	{
		char buff[100] = {0,};
		len = read(log_fds[0], buf, sizeof(buf));
		if ((fp = fopen(LOG_FILE, "a")) != NULL)
		{
			fprintf(fp, "%s", buf);
			fclose(fp);
		}
		else
		{
			perror("Fail to create log.txt");
		}
	}
	return NULL;
{


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


