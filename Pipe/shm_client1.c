#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include "sharemem.h"
#include "pv.h"
#include "p.c"
#include "v.c"
#include "initsem.c"

#define SHMKEY (key_t) 60100
#define SEMKEY 1111

void initMenu();
void initNcurses();
void gameRoom();
void waitingRoom();
void initshm();
void* checkWaitingRoomPlayer2Status();
void* checkGameRoomPlayer2TurnEnd();
int shmid, semid;

pthread_t waiting_thread, game_thread;
data_buf* shared_mem;

char* waiting_status[] = {"Wait", "Join", "Ready"};

WINDOW* waiting_player2_status;

int main(){	
	semid = initsem(SEMKEY);
	initshm(SHMKEY);
	initMenu();
	
	return 0;
}

void initNcurses(){
	initscr();
	clear();
	noecho();
	cbreak();
	curs_set(0);
}

void initshm(){
	if((shmid = shmget(SHMKEY, sizeof(data_buf), 0666 | IFLAGS)) == -1)
                printf("shmget error");

        if((shared_mem = (data_buf *) shmat(shmid, 0, 0)) == ERRO)
                printf("shmat error");
}

void* checkWaitingRoomPlayer2Status(){
	mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
        wrefresh(waiting_player2_status);

	while(shared_mem->wait_msg.opponent_ready == 0){
                        if(shared_mem->wait_msg.opponent_connect == 0){
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
                                wrefresh(waiting_player2_status);
                        }
                        else if(shared_mem->wait_msg.opponent_connect == 1){
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[1]);
                                wrefresh(waiting_player2_status);
                        }

                        if(shared_mem->wait_msg.opponent_ready == 1){
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[2]);
                                wrefresh(waiting_player2_status);
                        }
	}
	pthread_exit(NULL);
}

void* checkGameRoomPlayer2TurnEnd(){
	int xPoint = 3, xStart = 5, yStart = 3;
	int i, k;

	for(i = 0; i < ROW; i++){
                mvprintw(i + yStart, xStart, "|");
                for(k = 0; k < COLUMN; k++){
                        mvprintw(i + yStart,  1 + xPoint * k + xStart, "-%c-", shared_mem->game_msg.omok_board[i][k]);
                }
                mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
        }
	refresh();

        while(1){
                if(shared_mem->game_msg.my_turn != 0){
                        for(i = 0; i < ROW; i++){
                                mvprintw(i + yStart, xStart, "|");
                                for(k = 0; k < COLUMN; k++){
                                        mvprintw(i + yStart,  1 + xPoint * k + xStart, "-%c-", shared_mem->game_msg.omok_board[i][k]);
                                }
                                mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
                        }
			refresh();

        	        break;
		}
        }

}

void waitingRoom(){
	initNcurses();

	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	char* select[] = {"Ready!!", "Exit"};

	p(semid);
	shared_mem->wait_msg.connect = 1;
	shared_mem->wait_msg.status_change = 1;
	clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
	v(semid);

	WINDOW* player1 = newwin(5, 15, yStart, xStart);
        box(player1, 0, 0);
        wattron(player1, A_BOLD);
        mvwprintw(player1, 0, 2, "player 1(Me)");
        wattroff(player1, A_BOLD);

        WINDOW* player2 = newwin(5, 15, yStart, xStart + 15);
        box(player2, 0, 0);
        wattron(player2, A_BOLD);
        mvwprintw(player2, 0, 4, "player 2");
        wattroff(player2, A_BOLD);

	WINDOW* player1_status = newwin(1, 7, yStart + 2, xStart + 5);
	mvwprintw(player1_status, 0, 0, "%s", waiting_status[1]);

	waiting_player2_status = newwin(1, 7, yStart + 2, xStart + 20);

        WINDOW* ready_exit_box = newwin(3, 30, yStart + 5, xStart);
        box(ready_exit_box, 0, 0);

        keypad(ready_exit_box, TRUE);

        refresh();
	
        wrefresh(player1);
	wrefresh(player1_status);
        wrefresh(player2);
        wrefresh(ready_exit_box);

	pthread_create(&waiting_thread, NULL, checkWaitingRoomPlayer2Status, NULL);
        while(1){
                for(i = 0; i < 2; i++){
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
	if (highlight == 0){
		wattron(ready_exit_box, A_NORMAL);
	        mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
	        wattroff(ready_exit_box, A_NORMAL);
	        wrefresh(ready_exit_box);
	
	        touchwin(player1_status);
	        mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
	        wrefresh(player1_status);
	
	        p(semid);
	        shared_mem->wait_msg.ready = 1;
	        shared_mem->wait_msg.status_change = 1;
	        v(semid);
	
	        pthread_join(waiting_thread, NULL);
	
		WINDOW* start = newwin(5, 30, yStart + 5, xStart);
		for(int i = 0; i < 3; i++){
			mvwprintw(start, 0, 0, "Start in %d seconds!!", 3 - i);
			wrefresh(start);
			sleep(1);
		}

		endwin();
		gameRoom();
	}

	else{
		p(semid);
		clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
		shared_mem->wait_msg.connect = 0;
        	shared_mem->wait_msg.ready = 0;
        	shared_mem->wait_msg.status_change = 0;
       		shared_mem->wait_msg.opponent_connect = 0;
        	shared_mem->wait_msg.opponent_ready = 0;
        	shared_mem->wait_msg.opponent_change = 0;
		v(semid);

		endwin();
		return;
	}
}

void gameRoom(){
	initNcurses();
	
	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;

	move(yStart , xStart + 2);
	
	keypad(stdscr, TRUE);	
	
	refresh();

	while(1){
		pthread_create(&game_thread, NULL, checkGameRoomPlayer2TurnEnd, NULL);
		pthread_join(game_thread, NULL);

		if(shared_mem->game_msg.result == 2){
			mvprintw(yStart + 15, xStart, "Lose...");
			refresh();
                        for(int i = 0; i < 3; i++){
                                mvprintw(yStart + 15 + 1, xStart, "End in %d seconds", 3 - i);
                        	refresh();
				sleep(1);
                        }

			endwin();
                	return;
		}

		attron(A_REVERSE);
                mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", shared_mem->game_msg.omok_board[row][column]);
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
		if(c == 10 || c == ' '){
			if(shared_mem->game_msg.my_turn == 1 && shared_mem->game_msg.omok_board[row][column] == '+'){
				p(semid);
				shared_mem->game_msg.omok_board[row][column] = 'O';
				shared_mem->game_msg.turn_end = 1;
				shared_mem->game_msg.row = row;
				shared_mem->game_msg.col = column;
				clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
				v(semid);

				usleep(100000);
			
				if(shared_mem->game_msg.result == 1){
					mvprintw(yStart + 15 , xStart, "Win!!!");
					refresh();

					for(int i = 0; i < 3; i++){
						mvprintw(yStart + 15 + 1, xStart, "End in %d seconds", 3 - i);
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

void initMenu(){
	initNcurses();

	char* select[] = {"Game Start", "Exit"};

	int xStart = 8, yStart = 4;
	int highlight = 0;
	int c, i;

	WINDOW *menu_win;
	
	menu_win = newwin(6, 21, yStart,  xStart);

	box(menu_win, 0, 0);

	keypad(menu_win, TRUE);
	
	wattron(menu_win, A_BOLD);
	mvwprintw(menu_win, 0, 6, "Omok Game");
	wattroff(menu_win, A_BOLD);
	refresh();
	wrefresh(menu_win);

	while(1){
		refresh();
		wrefresh(menu_win);

		for(i = 0; i < 2; i++){
			if(highlight == i)
				wattron(menu_win, A_REVERSE);
			
			mvwprintw(menu_win, 2 + i, 6, "%s", select[i]);
			wattroff(menu_win, A_REVERSE);
		}

		c = wgetch(menu_win);

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
		if(c == 10 || c == ' ') break;
	}
	
	if(highlight == 0)
        	waitingRoom();
        
	else{
        	endwin();
        	return;
	}


}

