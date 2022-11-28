#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TO_SERVER_FILE "./fifo/from_client"
#define FROM_SERVER_FILE "./fifo/to_client"
#define USER_ID_SIZE 21
#define BUF_SIZE 1024
#define SERVER 1
#define CONNECT 327687 +1
#define PATH_SIZE 50
#define ROW 15
#define COLUMN 15

typedef struct msgtype{
	long mtype;
	char data[BUF_SIZE];
	long src;
}msg_t;

long id = 0;
int read_fd = 0;
int write_fd = 0;
char read_path[PATH_SIZE];
char omok_board[ROW][COLUMN];

void initMenu();
void initNcurses();
void gameRoom();
void waitingRoom();
void* checkWaitingRoomOpponentStatus();
void* checkGameRoomOpponentTurnEnd();


pthread_t waiting_thread, game_thread;

char* waiting_status[] = {"Wait", "Join", "Ready"};

WINDOW* waiting_opponent_status;

int main()
{
	id = (long)getpid();

	if ((write_fd = open(TO_SERVER_FILE, O_RDWR)) == -1)
	{
		perror("[SYSTEM] mkfifo file open failed(send to server)\n");
		printf("[SYSTEM] program exit in 3 sec..\n");
		sleep(3);
		exit(1);
	}
	sprintf(read_path, "%s%ld", FROM_SERVER_FILE, id);
	if ((read_fd = open(read_path, O_RDWR)) == -1)
	{
		perror("[SYSTEM] mkfifo file open failed(receive from server)\n");
		printf("[SYSTEM] mkfifo file creating...(receive from server)\n");
		mkfifo(read_path, 06666);
		printf("[SYSTEM] mkfifo file create complete!(receive from server)\n");
		if((read_fd = open(read_path, O_RDWR)) == -1)
		{	
		perror("[SYSTEM] mkfifo file open failed(receieve from server)\n");
			printf("[SYSTEM] program exit in 3 sec..\n");
			sleep(3);
			exit(1);
		}
	}

	initMenu();

	return 0;
}


void initMenu(){
	initNcurses();

	char* select[] = {"Game Start", "Exit"};

	int xStart= 8, yStart = 4;
	int highlight = 0;
	int c, i;

	WINDOW *menu_window;
	menu_window = newwin(6, 21, yStart, xStart);

	keypad(menu_window, TRUE);
	
	wattron(menu_window, A_BOLD);
	mvwprintw(menu_window, 0, 6, "Omok Game");
	wattroff(menu_window, A_BOLD);

	refresh();
	wrefresh(menu_window);
	
	while(1)
	{
		refresh();
		wrefresh(menu_window);

		for (i = 0; i < 2; i++)
		{
			if (highlight == i)
				wattron(menu_window, A_REVERSE);

			mvwprintw(menu_window, 2+i, 6, "%s", select[i]);
			wattroff(menu_window, A_REVERSE);
		}

		c = wgetch(menu_window);

		switch(c){
			case KEY_UP:
				if(highlight == 0) highlight = 1;
				highlight--;
				break;
			case KEY_DOWN:
				if(highlight == 1) highlight = 0;
				highlight++;
				break;
			default:
				break;
		}

		if (c == 10 || c == ' ') break;
	}
	if (highlight == 0)
		waitingRoom();

	else{
		endwin();
		return;
	}
}

void initNcurses()
{
	initscr();
	clear();
	noecho();
	cbreak();
	curs_set(0);
}

void gameRoom()
{
	initNcurses();

	msg_t msg;
	char send_buf[BUF_SIZE];
	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;

	move(yStart, xStart + 2);

	keypad(stdscr, TRUE);

	refresh();

	while(1)
	{
		pthread_create(&game_thread, NULL, checkGameRoomOpponentTurnEnd, NULL);
		pthread_join(game_thread, NULL);

		if(read(read_fd, &msg, sizeof(msg)) == -1)
		{
			perror("[SYSTEM] Read Error!!\n");
		}
		else
		{	
			if (*msg.data == '3')
			{
				mvprintw(yStart + 15, xStart, "Lose...");
				refresh();
			
				for (int i = 0; i < 3; i++)
				{
					mvprintw(yStart + 15 + 1, xStart, "End on %d seconds", 3 - i);
					refresh();
					sleep(1);
				}
				endwin();
				return;
			}
		}

		attron(A_REVERSE);
		mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", omok_board[row][column]);
		attroff(A_REVERSE);

		int c;

		c = getch();

		switch(c)
		{
			case KEY_UP:
				if(row == 0)
					row = 14;
				else
					row--;
				break;
			case KEY_DOWN:
				if(row == 14)
					row = 0;
				else
					row++;
				break;
			case KEY_LEFT:
				if(column == 0)
					column = 14;
				else
					column--;
				break;
			case KEY_RIGHT:
				if(column == 14)
					column = 0;
				else
					column++;
				break;
			default:
				break;
		}

		if (c == 10 || c == ' ')
		{
			if(omok_board[row][column] == '+')
			{
				omok_board[row][column] = 'O';
				memset(msg.data, '\0',sizeof(send_buf));
				send_buf[0] = (char)row;
				send_buf[1] = (char)column;
				strncpy(msg.data, send_buf, sizeof(send_buf));
				write(write_fd, &msg, sizeof(msg));
			}
			usleep(100000);

			if(read(read_fd, &msg, sizeof(msg)) == -1)
	                {
        	                perror("[SYSTEM] Read Error!!\n");
		       	}
                	else
                	{
                        	if (*msg.data == '4')
                        	{
                                	mvprintw(yStart + 15, xStart, "Win!!");
                                	refresh();

                                	for (int i = 0; i < 3; i++)
                                	{
                                        	mvprintw(yStart + 15 + 1, xStart, "End on %d seconds", 3 - i);
                                        	refresh();
                                        	sleep(1);
                                	}
                                	endwin();
                                	return;
                        	}	
                	}

		}
	}
	endwin();
}	


void waitingRoom()
{
	initNcurses();

	msg_t msg;
	char send_buf[BUF_SIZE];
	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	msg.mtype = SERVER;
	msg.src = id;

	char* select[] = {"Ready!!", "Exit"};

	memset(send_buf, '\0', sizeof(send_buf));
	send_buf[0] = '1';
	send_buf[strlen(send_buf)] = '\0';
	strncpy(msg.data,send_buf, sizeof(send_buf));
	write(write_fd, &msg, sizeof(msg));

	WINDOW* player1 = newwin(5, 15, yStart, xStart);
	box(player1, 0, 0);
	wattron(player1, A_BOLD);
	mvwprintw(player1, 0, 2, "player 1(Me)");
	wattroff(player1, A_BOLD);

	WINDOW* player2 = newwin(5, 15, yStart, xStart + 15);
	box(player2, 0,0);
	wattron(player2, A_BOLD);
	mvwprintw(player2, 0, 4, "player 2");
	wattroff(player2, A_BOLD);

	WINDOW* player1_status = newwin(1, 7, yStart + 2, xStart + 5);
	mvwprintw(player1_status, 0, 0, "%s", waiting_status[1]);

	waiting_opponent_status = newwin(1, 7, yStart + 2, xStart + 20);

	WINDOW* ready_exit_box = newwin(3, 30, yStart + 5, xStart);
	box(ready_exit_box, 0, 0);

	keypad(ready_exit_box, TRUE);

	refresh();

	wrefresh(player1);
	wrefresh(player1_status);
	wrefresh(player2);
	wrefresh(ready_exit_box);

	pthread_create(&waiting_thread, NULL, checkWaitingRoomOpponentStatus, NULL);
	while(1)
	{
		for (i = 0; i < 2; i++)
		{
			if(highlight == i)
				wattron(ready_exit_box, A_REVERSE);

			mvwprintw(ready_exit_box, 1, 5 + i * 15, "%s", select[i]);
			wattroff(ready_exit_box, A_REVERSE);
		}

		int c;

		c = wgetch(ready_exit_box);

		switch(c){
			case KEY_LEFT:
				if(highlight == 0) highlight = 1;
				highlight--;
				break;
			case KEY_RIGHT:
				if(highlight == 1) highlight = 0;
				highlight++;
				break;
			default:
				break;
		}

		if(c == 10 || c == ' ') break;
	}

	if (highlight == 0)
	{
		wattron(ready_exit_box, A_NORMAL);
		mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
		wattroff(ready_exit_box, A_NORMAL);
		wrefresh(ready_exit_box);

		touchwin(player1_status);
		mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
		wrefresh(player1_status);

		memset(send_buf, '\0', sizeof(send_buf));
                send_buf[0] = '2';
		send_buf[strlen(send_buf)] = '\0';
		strncpy(msg.data,send_buf, sizeof(send_buf));
	        write(write_fd, &msg, sizeof(msg));

		pthread_join(waiting_thread, NULL);

		WINDOW* start = newwin(5, 30, yStart + 5, xStart);
		for(int i = 0; i < 3; i ++)
		{
			mvwprintw(start, 0, 0, "Start in %d seconds!!", 3 - i);
			wrefresh(start);
			sleep(1);
		}
		endwin();
		gameRoom();
	}
	else
	{
		memset(send_buf, '\0', sizeof(send_buf));
		send_buf[0] = '0';
		send_buf[strlen(send_buf)] = '\0';
		strncpy(msg.data,send_buf, sizeof(send_buf));
		write(write_fd, &msg, sizeof(msg));
		endwin();
		return;
	}
}

void* checkWaitingRoomOpponentStatus()
{
	msg_t msg;
	char opponent_ready = '\0';
	char opponent_connect = '\0';

	mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[0]);
	wrefresh(waiting_opponent_status);

	if(read(read_fd, &msg, sizeof(msg)) == -1)
	{
		perror("[SYSTEM] Read Error\n");
	}
	else
	{
		while (*msg.data == '2')
		{
			if (*msg.data == '0')
			{
				touchwin(waiting_opponent_status);
				mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[0]);
				wrefresh(waiting_opponent_status);
			}
			else if (*msg.data == '1')
			{
				touchwin(waiting_opponent_status);
				mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[1]);
				wrefresh(waiting_opponent_status);
			}

			else if (*msg.data == '2')
			{
				touchwin(waiting_opponent_status);
				mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[2]);
				wrefresh(waiting_opponent_status);
			}
		}
	}
	pthread_exit(NULL);	
}

void* checkGameRoomOpponentTurnEnd()
{
	int xPoint = 3, xStart = 5, yStart = 3;
	int i, k;
	char a;
	msg_t msg;

	for (i = 0; i < ROW; i++)
	{
		mvprintw(i + yStart, xStart, "|");
		for(k = 0; k < COLUMN; k++)
		{
			mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-",omok_board[i][k]);
		}
		mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
	}
	refresh();


	while(1)
	{
		memset(msg.data, '\0', BUF_SIZE);
		if (read(read_fd, &msg, sizeof(msg)) == -1)
		{
			perror("[SYSTEM] Read Error");
		}
		else
		{
			if (*msg.data == '5')
			{
				for (i=0; i < ROW; i++)
				{
					mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-", omok_board[i][k]);
				}
				mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
			}
			refresh();

			break;
		}
	}
}
