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
void printRatingTransferRate(struct timespec start, struct timespec end);
void* gameRoomDataCommunication();
void* judgeOmok(void* g);

data_msg send_msg;
data_msg recv_msg;

int pid[2];
int read_fd, write_fd[2];
char path[PATH_SIZE];

int main(){
	pthread_t game_thread, waiting_thread;
	void* game_thread_return;
	int i, fd;

	pthread_create(&waiting_thread, NULL, waitingRoomDataCommunication, NULL);
	pthread_join(waiting_thread, NULL);

	pthread_create(&game_thread, NULL, gameRoomDataCommunication, NULL);
	pthread_join(game_thread, NULL);

	close(read_fd);
	close(write_fd[0]);
	close(write_fd[1]);
	
	memset(path, '\0', PATH_SIZE);
	snprintf(path, PATH_SIZE, "%s%d", TO_CLIENT_FILE, pid[0]);
	remove(path);

	memset(path, '\0', PATH_SIZE);
	snprintf(path, PATH_SIZE, "%s%d", TO_CLIENT_FILE, pid[1]);
	remove(path);
	remove(FROM_CLIENT_FILE);

	return 0;
}

void* waitingRoomDataCommunication(){
	int i, j;
	int player1_ready = 0, player2_ready = 0;
	struct timespec end;

	system("clear");

	mkfifo(FROM_CLIENT_FILE, 0666);

	for(i = 0; i < 2; i++){
		if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1){
                        printf("read_fd open error\n");
                }

                else if (read(read_fd, &recv_msg, sizeof(data_msg)) == -1){
                        perror("read error\n");
                }
                else{
			pid[i] = recv_msg.pid;
			clock_gettime(CLOCK_MONOTONIC, &end);		
			printRatingTransferRate(recv_msg.start, end);

			printf("player%d connected\n", i + 1);
		}
	
	}

	for(i = 0; i < 2; i++){
		memset(path, '\0', PATH_SIZE);
		snprintf(path,PATH_SIZE, "%s%d", TO_CLIENT_FILE, pid[i]);
                mkfifo(path, 0666);
		if((write_fd[i] = open(path, O_RDWR)) == -1){
			printf("write_fd open error\n");
		}
                send_msg.w_msg.oppenent_connect = 1;
        	write(write_fd[i], &send_msg, sizeof(send_msg));		
	}

	for(i = 0; i < 2; i++){
                if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1){
                        printf("read_fd open error\n");
                }

                else if (read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
                        printf("read error\n");
                }
                else{
			if(recv_msg.w_msg.ready == 1 && recv_msg.pid == pid[0]){
				clock_gettime(CLOCK_MONOTONIC, &end);
	                        printRatingTransferRate(recv_msg.start, end);

				send_msg.w_msg.oppenent_ready = 1;
                                write(write_fd[1], &send_msg, sizeof(send_msg));

				printf("player1 is ready\n");
			}
			else if(recv_msg.w_msg.ready == 1 && recv_msg.pid == pid[1]){
				clock_gettime(CLOCK_MONOTONIC, &end);
	                        printRatingTransferRate(recv_msg.start, end);

				send_msg.w_msg.oppenent_ready = 1;
                                write(write_fd[0], &send_msg, sizeof(send_msg));

				printf("player2 is ready\n");
			}
                }
        }
	printf("game start\n");

}

void printRatingTransferRate(struct timespec start, struct timespec end){
	double accum;

        accum = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000;
        printf("[Transfer Rate] %.9lf\n",accum);
}

void* gameRoomDataCommunication(){
	void* ret;
	int game_end = 0;
	struct timespec time_end;
	pthread_t judge_thread;

	while(game_end == 0){
		for(int i = 0; i < 2; i++){
			send_msg.g_msg.my_turn = 1;
			write(write_fd[i], &send_msg, sizeof(send_msg));

			send_msg.g_msg.my_turn = 0;
			write(write_fd[(i + 1) % 2], &send_msg, sizeof(send_msg));

			if(read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
				printf("read_fd error\n");
			}
			else{
				clock_gettime(CLOCK_MONOTONIC, &time_end);
				printRatingTransferRate(recv_msg.start, time_end);

				pthread_create(&judge_thread, NULL, judgeOmok, (void*)&recv_msg.g_msg);
                                pthread_join(judge_thread,(void*) &ret);

				if(*(int*)ret == 1){
					send_msg.g_msg.result = 1;
					write(write_fd[i], &send_msg, sizeof(send_msg));

					send_msg.g_msg.row = recv_msg.g_msg.row;
                                	send_msg.g_msg.col = recv_msg.g_msg.col;
					send_msg.g_msg.result = 2;
					write(write_fd[(i + 1) % 2], &send_msg, sizeof(send_msg));

					game_end = 1;

					break;
				}
				else{
					send_msg.g_msg.result = 0;
                                        write(write_fd[i], &send_msg, sizeof(send_msg));

					send_msg.g_msg.row = recv_msg.g_msg.row;
                                	send_msg.g_msg.col = recv_msg.g_msg.col;
					write(write_fd[(i + 1) % 2], &send_msg, sizeof(send_msg));
				}
			}

		}
	}
	printf("game is ended\n");
}

void* judgeOmok(void* g){
	int i, k, cnt = 0;
	int row, col, ret = 0;
	char board[ROW][COLUMN];

	game_msg game= *((game_msg*)g);

	for(i = 0; i < ROW; i++){
		for(k = 0; k < COLUMN; k++){
			board[i][k] = game.omok_board[i][k];
		}
	}
	row = game.row;
	col = game.col;


	for(i = 0; i < ROW; i++){
		if(board[i][col] == 'O') cnt++;
		else cnt = 0;

		if(cnt == 5){
			if(i + 1 == ROW || board[i + 1][col] != 'O'){
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
                         if(i + 1 == COLUMN || board[row][i + 1] != 'O'){
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
                          if(start_row + i + 1 == ROW || board[start_row + i + 1][start_col + i + 1] != 'O'){
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

			if(cnt == 5){
                          if(start_col + i + 1 == COLUMN || board[start_row + i + 1][start_col + i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

                }

	}

	start_row = row + col;
	start_col = 0;
	cnt = 0;
	for(i = 0; start_row - i >= 0; i++){
		if(board[start_row - i][i] == 'O') cnt++;
		else cnt = 0;

		if(start_row - i - 1 >= 0 && cnt == 5){
                          if(start_row - i - 1 == -1 || board[start_row - i - 1][i + 1] != 'O'){
                                  ret = 1;
                                  pthread_exit((void*)&ret);
                          }
                  }

	}

	ret = 0;
	pthread_exit((void*)&ret);

}
