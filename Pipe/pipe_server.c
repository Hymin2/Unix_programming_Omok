#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FROM_CLIENT_FILE "./fifo/from_client"
#define TO_CLIENT_FILE "./fifo/to_client_"
#define USER_ID_SIZE 21
#define BUF_SIZE 1024
#define SERVER 1
#define CONNECT 32768+1
#define PATH_SIZE 50
#define ROW 15
#define COLUMN 15

typedef struct msgtype{
	long mtype;
	char data[BUF_SIZE];
	long src;
}msg_t;

typedef struct Room{
	long clnt[2];
}Room;

typedef struct Ready{
	long clnt[2];
}Ready;

void rcvConnectionMsg(Room* room);
void startUserChatting(Room* room);
void* writeLogToTextFile();

void* waitingRoomDataCommunication();
void* gameRoomDataCommunication();
void* judgeOmok(void* turn);

Room room;
Ready ready;

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

void* waitingRoomDataCommunication()
{
	int i, j;
	int player1_ready = 0, player2_ready = 0;
	int read_fd, write_fd;
	char send_buf[BUF_SIZE];
	msg_t msg;
	char path[PATH_SIZE];

	system("clear");

	mkfifo(FROM_CLIENT_FILE, 0666);

	for (i = 0; i < 2; i++)
	{
		memset(msg.data, 0, BUF_SIZE);

		if ((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1)
		{
			perror("[SYSTEM] Open Error!!\n");
		}
		
		else if (read(read_fd, &msg, sizeof(msg)) == -1)
		{
			perror("[SYSTEM] Read Error!!\n");
		}
		else
		{

			room.clnt[i] = msg.src;
			sprintf(path, "%s%ld", TO_CLIENT_FILE, room.clnt[i]);
			mkfifo(path, 0666);
			memset(msg.data, 0, BUF_SIZE);
			send_buf[0] = '1';
			strncpy(msg.data, send_buf, sizeof(send_buf));
			write(write_fd, &msg, sizeof(msg));

			if((read_fd = open(FROM_CLIENT_FILE, O_RDWR)) == -1)
			{
				perror("[SYSTEM] Open Error!!\n");
			}
			else if (read(read_fd, &msg, sizeof(msg)) == -1)						 {
					perror("[SYSTEM] Read Error!!\n");
			}
			else
			{

				ready.clnt[i] = msg.src;
				//sprintf(path,"%s%ld", TO_CLIENT_FILE, ready.clnt[j]);
				mkfifo(path, 0666);
				memset(msg.data, 0, BUF_SIZE);
				send_buf[0]='2';
				strncpy(msg.data, send_buf, sizeof(send_buf));
				write(write_fd, &msg, sizeof(msg));

			}

		}
	}
}

/*
void* gameRoomDataCommunication()
{
	void* ret;
	int turn;

	
	pthread_t judge_thread;
	


	while(1)
	{
		while(1)
		{
			mem		

			pthread_create(&judge_thread, judgeOmok, (void*)turn);
			pthread_join(jundge_thread, (void*) &ret);

			break;
		}
	}

	if (*(int*)ret == 1)
	{


		break;
	}

	while(1)
	{
		if()
		{
			
		}
	}
}


void* judgeOmok(void* turn)
{
	int i, k, cnt = 0, t = *((int*)turn);
	int row, col, ret = 0;
	char board[ROW][COLUMN];

	if (t == 1)
	{
		for (i = 0; i < ROW; i++)
		{
			for (k = 0; k < COLUMN; k++)
			{
				board[i][k] = ;
			}
		}
		row = ;
		col = ;
	}
}
*/
