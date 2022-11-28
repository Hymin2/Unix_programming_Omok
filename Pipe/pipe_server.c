#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>


#define FROM_CLIENT_FILE "./from_client"
#define TO_CLIENT_FILE "./to_client_"
#define USER_ID_SIZE 21
#define BUF_SIZE 1024
#define SERVER 1
#define CONNECT 32768+1
#define PATH_SIZE 50
#define ROW 15
#define COLUMN 15

typedef struct waitingroom_msg{
	int ready;
	int oppenent_connect;
	int oppenent_ready;
}waiting_msg;

typedef struct gameroom_msg{
	int row;
	int col;
	int my_turn;
	int turn_end;
	int result;
	char omok_board[ROW][COLUMN];
}game_msg;

typedef struct data_msg{
	int pid;
	waiting_msg w_msg;
	game_msg g_msg;
	struct timespec start;
}data_msg;

void* waitingRoomDataCommunication();
void printRatingTransferRate(struct timespec start);

data_msg send_msg;
data_msg recv_msg;

int pid[2];
int read_fd, write_fd[2];

int main()
{
	pthread_t game_thread, waiting_thread;
	void* game_thread_return;
	int i, fd;

	pthread_create(&waiting_thread, NULL, waitingRoomDataCommunication, NULL);
	pthread_join(waiting_thread, NULL);

	//pthread_create(&game_thread, NULL, gameRoomDataCommunication, NULL);
	//pthread_join(game_thread, NULL);


}

void* waitingRoomDataCommunication(){
	int i, j;
	int player1_ready = 0, player2_ready = 0;
	char path[PATH_SIZE];

	system("clear");

	mkfifo(FROM_CLIENT_FILE, 0666);

	for(i = 0; i < 2; i++){
		if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1){
                        perror("[SYSTEM] Open Error!!\n");
                }

                else if (read(read_fd, &recv_msg, sizeof(data_msg)) == -1){
                        perror("[SYSTEM] Read Error!!\n");
                }
                else{
			pid[i] = recv_msg.pid;

                        //clock_gettime(CLOCK_MONOTONIC, &recv_msg.end);
                        //accum = (recv_msg.end.tv_sec - recv_msg.start.tv_sec) + (double)(recv_msg.end.tv_nsec - recv_msg.start.tv_nsec) / 1000000000;
                        //printf("%.9f\n",accum);
			printRatingTransferRate(recv_msg.start);
		}
	
	}

	for(i = 0; i < 2; i++){
		sprintf(path, "%s%d", TO_CLIENT_FILE, pid[i]);
                mkfifo(path, 0666);
		if((write_fd[i] = open(path, O_RDWR)) == -1){
			printf("write_fd open error\n");
		}
                send_msg.w_msg.oppenent_connect = 1;
        	write(write_fd[i], &send_msg, sizeof(send_msg));		
	}

	for(i = 0; i < 2; i++)
        {
                if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1)
                {
                        perror("[SYSTEM] Open Error!!\n");
                }

                else if (read(read_fd, &recv_msg, sizeof(recv_msg)) == -1)
                {
                        perror("[SYSTEM] Read Error!!\n");
                }
                else
                {
			if(recv_msg.w_msg.ready == 1 && recv_msg.pid == pid[0]){
                                //clock_gettime(CLOCK_MONOTONIC, &recv_msg.end);
        	                //accum = (recv_msg.end.tv_sec - recv_msg.start.tv_sec) + (double)(recv_msg.end.tv_nsec - recv_msg.start.tv_nsec) / 1000000000;
	                        //printf("%.9f\n",accum);

	                        printRatingTransferRate(recv_msg.start);

				send_msg.w_msg.oppenent_ready = 1;
                                write(write_fd[1], &send_msg, sizeof(send_msg));
			}
			else if(recv_msg.w_msg.ready == 1 && recv_msg.pid == pid[1]){
                		//clock_gettime(CLOCK_MONOTONIC, &recv_msg.end);
        	                //accum = (recv_msg.end.tv_sec - recv_msg.start.tv_sec) + (double)(recv_msg.end.tv_nsec - recv_msg.start.tv_nsec) / 1000000000;
	                        //printf("%.9f\n",accum);

	                        printRatingTransferRate(recv_msg.start);

				send_msg.w_msg.oppenent_ready = 1;
                                write(write_fd[0], &send_msg, sizeof(send_msg));
			}
                }
        }

}

void printRatingTransferRate(struct timespec start){
	struct timespec end;
	double accum;

	clock_gettime(CLOCK_MONOTONIC, &end);
        accum = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000;
        printf("%.9f\n",accum);
}

void* gameRoomDataCommunication(){
	void* ret;
	int turn;
	pthread_t judge_thread;

	while(1){

		for(int i = 0; i < 2; i++){
			send_msg.g_msg.my_turn = 1;	
		}


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
