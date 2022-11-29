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

#define TO_SERVER_FILE "./from_client"
#define FROM_SERVER_FILE "./to_client_"
#define USER_ID_SIZE 21
#define BUF_SIZE 1024
#define SERVER 1
#define CONNECT 327687 +1
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

data_msg recv_msg;
data_msg send_msg;

int read_fd;
int write_fd;

char read_path[PATH_SIZE];

void printOmokBoard();
void initMenu();
void initNcurses();
void gameRoom();
void waitingRoom();
void* checkWaitingRoomOpponentStatus();
void* checkGameRoomOpponentTurnEnd();
void* checkGameRoomMyTurn();
void* connectToServer();

pthread_t waiting_thread;

char* waiting_status[] = {"Wait", "Join", "Ready"};

WINDOW* waiting_opponent_status;

int main()
{
	initMenu();

	return 0;
}

void* connectToServer(){
	int pid;

	pid = getpid();

	if ((write_fd = open(TO_SERVER_FILE, O_RDWR)) == -1){
                perror("write_fd open error\n");
                sleep(3);
                exit(1);
        }

	sprintf(read_path, "%s%d", FROM_SERVER_FILE, pid);

        send_msg.pid = pid;
        clock_gettime(CLOCK_MONOTONIC, &send_msg.start);
        write(write_fd, &send_msg, sizeof(send_msg));

        if ((read_fd = open(read_path, O_RDWR)) == -1){
                perror("read_fd open error\n");
                mkfifo(read_path, 0666);

                if((read_fd = open(read_path, O_RDWR)) == -1){
                         perror("read_fd open error\n");
                        printf("[SYSTEM] program exit in 3 sec..\n");
                        sleep(3);
                        exit(1);
                }
        }
	
}

void gameRoom()
{
	initNcurses();
	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;
	void*ret;

	pthread_t check_myturn_thread, check_oppenent_turnend_thread;

	move(yStart, xStart + 2);
	keypad(stdscr, TRUE);

	for (i = 0; i < ROW; i++) {
		for (k = 0; k < COLUMN; k++) {
			send_msg.g_msg.omok_board[i][k] = '+';
		}
	}

	printOmokBoard();

	while(1){
		pthread_create(&check_myturn_thread, NULL, checkGameRoomMyTurn, NULL);
		pthread_join(check_myturn_thread, &ret);

		if(*(int*)ret == 0){
			pthread_create(&check_oppenent_turnend_thread, NULL, checkGameRoomOpponentTurnEnd, NULL);
			pthread_join(check_oppenent_turnend_thread, &ret);
			printOmokBoard();

			if (*(int*)ret == 1){
                                mvprintw(yStart + 15, xStart, "Lose...");
                                refresh();

                                for (int i = 0; i < 3; i++){
                                        mvprintw(yStart + 15 + 1, xStart, "End on %d seconds", 3 - i);
                                        refresh();
                                        sleep(1);
                                }
                                endwin();
                        return;
                        }

		}

		else{
			while(1){
				printOmokBoard();
				attron(A_REVERSE);
				mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", send_msg.g_msg.omok_board[row][column]);
				attroff(A_REVERSE);

				int c;
				c = getch();
				switch(c){
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
				if (c == 10 || c == ' '){
					if(send_msg.g_msg.omok_board[row][column] == '+'){
						send_msg.g_msg.omok_board[row][column] = 'O';
						send_msg.g_msg.row = row;
						send_msg.g_msg.col = column;
						clock_gettime(CLOCK_MONOTONIC, &send_msg.start);
						write(write_fd, &send_msg, sizeof(send_msg));

						printOmokBoard();

					
						usleep(100000);
						if(read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
	       						perror("[SYSTEM] Read Error!!\n");
						}
						
						else{
							if (recv_msg.g_msg.result == 1){
								mvprintw(yStart + 15, xStart, "Win!!");
								refresh();
								for (int i = 0; i < 3; i++){
									mvprintw(yStart + 15 + 1, xStart, "End on %d seconds", 3 - i);
									refresh();
									sleep(1);
								}
							endwin();
							return;
							}
						}

						break;
					}
					
				}	
			}
		}
	}
	endwin();
}	

void printOmokBoard() {
	int xStart = 5, yStart = 3, xPoint = 3;
	for (int i = 0; i < ROW; i++) {
		mvprintw(i + yStart, xStart, "|");
		for (int k = 0; k < COLUMN; k++) {
			mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-", send_msg.g_msg.omok_board[i][k]);
		}
		mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
	}
	refresh();
}

void* checkGameRoomMyTurn(){
	int ret = 0;

	if (read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
                perror("[SYSTEM] Read Error");
        }
        else{
                if (recv_msg.g_msg.my_turn == 1) {
                        ret = 1;
                        pthread_exit((void*)&ret);
                }
        }
        pthread_exit((void*)&ret);	
}

void* checkGameRoomOpponentTurnEnd()
{
	int ret = 0;

	if (read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
		perror("[SYSTEM] Read Error");
	}
	else{
		send_msg.g_msg.omok_board[recv_msg.g_msg.row][recv_msg.g_msg.col] = 'X';
		if (recv_msg.g_msg.result == 2) {
			ret = 1;
			pthread_exit((void*)&ret);
		}
	}
	pthread_exit((void*)&ret);
}

void initMenu(){
	initNcurses();

	char* select[] = {"Game Start", "Exit"};

	int xStart= 8, yStart = 4;
	int highlight = 0;
	int c, i;

	WINDOW *menu_window;
	menu_window = newwin(6, 21, yStart, xStart);
	box(menu_window, 0, 0);
	
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

void waitingRoom()
{
	initNcurses();
	char send_buf[BUF_SIZE];
	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	char* select[] = {"Ready!!"};

	connectToServer();

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

	WINDOW* ready_box = newwin(3, 15, yStart + 5, xStart);
	mvwprintw(ready_box, 0, 7, "Ready!!");
	box(ready_box, 0, 0);

	keypad(ready_box, TRUE);

	refresh();

	wrefresh(player1);
	wrefresh(player1_status);
	wrefresh(player2);
	wrefresh(ready_box);

	pthread_create(&waiting_thread, NULL, checkWaitingRoomOpponentStatus, NULL);

	int c;

	c = wgetch(ready_box);

	if(c == 10 || c == ' '){
		wattron(ready_box, A_NORMAL);
		mvwprintw(ready_box, 1, 5, "Ready!!");
		wattroff(ready_box, A_NORMAL);
		wrefresh(ready_box);
	
		touchwin(player1_status);
		mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
		wrefresh(player1_status);
	
		send_msg.w_msg.ready = 1;

	        write(write_fd, &send_msg, sizeof(send_msg));
	
		pthread_join(waiting_thread, NULL);
	
		WINDOW* start = newwin(5, 30, yStart + 5, xStart);
		for(int i = 0; i < 3; i ++){
			mvwprintw(start, 0, 0, "Start in %d seconds!!", 3 - i);
			wrefresh(start);
			sleep(1);
		}
		endwin();
		gameRoom();
	}

}

void* checkWaitingRoomOpponentStatus()
{
	mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[0]);
	wrefresh(waiting_opponent_status);

	for(int i = 0; i < 2; i++){
		
		if(read(read_fd, &recv_msg, sizeof(recv_msg)) == -1){
			perror("[SYSTEM] Read Error\n");
		}
		else{
			if(i == 0){
				mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[1]);
			        wrefresh(waiting_opponent_status);
			}
			else{
				mvwprintw(waiting_opponent_status, 0, 0, "%s", waiting_status[2]);
				wrefresh(waiting_opponent_status);
			}
		}
	}
	pthread_exit(NULL);	
}
