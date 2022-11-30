#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include "sharemem.h"
#include "pv.h"
#include "p.c"
#include "v.c"
#include "initsem.c"

#define SHMKEY (key_t) 60100		// 공유 메모리 키
#define SEMKEY 60102		// 세마포어 키

void initMenu();			// 첫 화면 메뉴 표시 함수
void initNcurses();			// Ncurses를 사용하기 위한 초기화 함수
void gameRoom();			// 게임 화면 함수
void waitingRoom();			// 대기실 화면 함수
void initshm();				// 공유 메모리 초기화 함수
void* checkWaitingRoomPlayer2Status();	// 대기실에서 상대방이 연결 상태를 확인해줄 쓰레드 함수
void* checkGameRoomPlayer2TurnEnd();	// 게임 화면에서 상대방의 턴이 끝났는지 확인해줄 쓰레드 함수
void printOmokBoard();

// 공유 메모리, 세마포어 아이디를 받을 변수
int shmid, semid;

// 쓰레드 변수
pthread_t waiting_thread, game_thread;
pthread_mutex_t mutex;
// 공유 메모리에서 사용할 변수
data_buf* shared_mem;

// 대기실 화면 상태 메세지
char* waiting_status[] = {"Wait", "Join", "Ready"};

// 대기실 화면에서 상대방의 상태를 표시해줄 윈도우
WINDOW* waiting_player2_status;

int main(){	
	semid = initsem(SEMKEY);	// 세마포어 초기화
	initshm();		// 공유 메모리 초기화
	initMenu();			// 초기 메뉴 화면 
	
	if(shmdt(shared_mem) < 0){
		printf("shmdt error\n");
	}
	return 0;
}

void initNcurses(){
	initscr();			// ncurses 사용 선언
	clear();			// 화면 다 지우기
	noecho();			// 사용자의 입력을 다시 출력하지 않음
	cbreak();			// cbreak모드 사용
	curs_set(0);			// 커서 깜빡임 없에기
}

void initshm(){
	// 공유 메모리 생성
	if((shmid = shmget(SHMKEY, sizeof(data_buf), 0666 | IFLAGS)) == -1)
                printf("shmget error");
	// 공유 메모리 부착
        if((shared_mem = (data_buf *) shmat(shmid, 0, 0)) == ERRO)
                printf("shmat error");
}

void* checkWaitingRoomPlayer2Status(){
	// 처음 player2의 상태를 Wait로 출력
	pthread_mutex_lock(&mutex);
	mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
        wrefresh(waiting_player2_status);
	pthread_mutex_unlock(&mutex);

	// player2의 상태가 ready가 될 때 까지 반복
	while(shared_mem->wait_msg.opponent_ready == 0){
			// player2가 연결이 안되어있으면 wait 상태 표시
                        if(shared_mem->wait_msg.opponent_connect == 0){
				pthread_mutex_lock(&mutex);
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[0]);
                                wrefresh(waiting_player2_status);
				pthread_mutex_unlock(&mutex);
                        }
			// player2가 연결되었으면 join 상태 표시
                        else if(shared_mem->wait_msg.opponent_connect == 1){
				pthread_mutex_lock(&mutex);
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[1]);
                                wrefresh(waiting_player2_status);
				pthread_mutex_unlock(&mutex);
                        }
			// player2가 준비를 하면 ready 상태 표시
                        if(shared_mem->wait_msg.opponent_ready == 1){
				pthread_mutex_lock(&mutex);
                                touchwin(waiting_player2_status);
                                mvwprintw(waiting_player2_status, 0, 0, "%s", waiting_status[2]);
                                wrefresh(waiting_player2_status);
				pthread_mutex_unlock(&mutex);
                        }
	}
	// 쓰레드 종료
	pthread_exit(NULL);
}

void printOmokBoard(){
	int xPoint = 3, xStart = 5, yStart = 3;
        int i, k;

        // 오목판 생성 15x15
        for(i = 0; i < ROW; i++){
                mvprintw(i + yStart, xStart, "|");
                for(k = 0; k < COLUMN; k++){
                        mvprintw(i + yStart,  1 + xPoint * k + xStart, "-%c-", shared_mem->game_msg.omok_board[i][k]);
                }
                mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
        }
        // 화면 새로고침
        refresh();
}

void* checkGameRoomPlayer2TurnEnd(){
	int ret = 0;

        while(1){
		// player2의 턴이 끝나면 오목판 새로고침
                if(shared_mem->game_msg.my_turn != 0 || shared_mem->game_msg.result != 0){
        	      if(shared_mem->game_msg.result == 2){
		      	ret = 1;
			pthread_exit((void*)&ret);
			break;
		      }
		      break;
		}
        }
	pthread_exit((void*)&ret);
}

void waitingRoom(){
	initNcurses();			// ncurses 초기화

	int i, highlight = 0;		// 선택한 메뉴 저장
	int xStart = 5, yStart = 3;	// (5,3)에서 시작

	// 대기실에서 선택 메뉴
	char* select[] = {"Ready!!", "Exit"};

	// 대기실에 연결했음을 서버와 공유 변수를 통해 알림
	p(semid);
	shared_mem->wait_msg.connect = 1;
	shared_mem->wait_msg.status_change = 1;
	clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
	v(semid);

	// player1의 상태창
	WINDOW* player1 = newwin(5, 15, yStart, xStart);
        box(player1, 0, 0);
        wattron(player1, A_BOLD);
        mvwprintw(player1, 0, 2, "player 1(Me)");
        wattroff(player1, A_BOLD);

	// player2의 상태창
        WINDOW* player2 = newwin(5, 15, yStart, xStart + 15);
        box(player2, 0, 0);
        wattron(player2, A_BOLD);
        mvwprintw(player2, 0, 4, "player 2");
        wattroff(player2, A_BOLD);

	// player1의 상태 메세지
	WINDOW* player1_status = newwin(1, 7, yStart + 2, xStart + 5);
	mvwprintw(player1_status, 0, 0, "%s", waiting_status[1]);

	// player2의 상태 메세지
	waiting_player2_status = newwin(1, 7, yStart + 2, xStart + 20);

	// 대기실의 선택 메뉴
        WINDOW* ready_exit_box = newwin(3, 30, yStart + 5, xStart);
	mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
	mvwprintw(ready_exit_box, 1, 20, "%s", select[1]);
        box(ready_exit_box, 0, 0);

	// 선택 메뉴에서 키보드 사용
        keypad(ready_exit_box, TRUE);

	// 화면 새로고침
        refresh();

	// player2가 연결 상태를 확인하는 쓰레드 생성
        pthread_create(&waiting_thread, NULL, checkWaitingRoomPlayer2Status, NULL);

	pthread_mutex_lock(&mutex);
        wrefresh(player1);
	wrefresh(player1_status);
        wrefresh(player2);
        wrefresh(ready_exit_box);
	pthread_mutex_unlock(&mutex);

        while(1){
		// 선택한 메뉴를 표시
		pthread_mutex_lock(&mutex);
		mvwprintw(ready_exit_box, 1, 5 + ((highlight + 1) % 2) * 15, "%s", select[(highlight + 1) % 2]);
                wattron(ready_exit_box, A_REVERSE);
	        mvwprintw(ready_exit_box, 1, 5 + highlight * 15, "%s", select[highlight]);
	        wattroff(ready_exit_box, A_REVERSE);
		wrefresh(ready_exit_box);
		pthread_mutex_unlock(&mutex);
			// 사용자의 키를 입력 받음
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
	
		// 엔터를 누르면 무한루프 종료
	        if(c == 10 || c == ' '){
			if(shared_mem->wait_msg.opponent_connect == 1)
				break;
			
		}	
        }
	
	// ready를 선택했을 때
	if (highlight == 0){
		// 선택한 메뉴의 표시 효과 없에기
		wattron(ready_exit_box, A_NORMAL);
	        mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
	        wattroff(ready_exit_box, A_NORMAL);
	        wrefresh(ready_exit_box);
	
		// player1의 상태를 ready로 변경
	        touchwin(player1_status);
	        mvwprintw(player1_status, 0, 0, "%s", waiting_status[2]);
	        wrefresh(player1_status);
	
		// 서버에 ready를 했음을 알림
	        p(semid);
	        shared_mem->wait_msg.ready = 1;
	        shared_mem->wait_msg.status_change = 1;
		clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
	        v(semid);
	
		// player2가 ready상태가 될 때 까지 대기
	        pthread_join(waiting_thread, NULL);
	
		// player 모두 ready를 하면 Start 메세지 출력
		WINDOW* start = newwin(5, 30, yStart + 5, xStart);
		for(int i = 0; i < 3; i++){
			mvwprintw(start, 0, 0, "Start in %d seconds!!", 3 - i);
			wrefresh(start);
			sleep(1);
		}
		
		endwin();
		
		// 게임 화면 진입
		gameRoom();
	}

	// Exit를 선택했을 때
	else{
		// player1의 상태를 초기화
		p(semid);
		clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
		shared_mem->wait_msg.connect = 0;
        	shared_mem->wait_msg.ready = 0;
        	shared_mem->wait_msg.status_change = 0;
       		shared_mem->wait_msg.opponent_connect = 0;
        	shared_mem->wait_msg.opponent_ready = 0;
        	shared_mem->wait_msg.opponent_change = 0;
		v(semid);

		// 종료
		endwin();
		return;
	}
}

void gameRoom(){
	initNcurses();			// ncurses 초기화
	
	int xStart = 5, yStart = 3;	// (5,3)에서 시작	
	int i, k, xPoint = 3;		// 오목판에서 x축으로 3칸씩 움직임 (-+-)
	int row = 0, column = 0;	// 사용자가 입력한 row, col이 저장
	void* ret;

	// 사용자의 커서를 (7, 3)으로 이동
	// 오목판의 좌측상단
	move(yStart , xStart + 2);
	
	// 사용자의 입력을 허용
	keypad(stdscr, TRUE);	
	
	printOmokBoard();

	while(1){
		// player2의 턴이 끝날 때 까지 대기
		pthread_create(&game_thread, NULL, checkGameRoomPlayer2TurnEnd, NULL);
		pthread_join(game_thread, &ret);
		printOmokBoard();
		
		usleep(100000);

		// player2의 승리일 경우
		if(*(int*)ret == 1){
			// 패배 메세지 출력 후 종료
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

		// 사용자의 선택하고 있는 곳 표시
		attron(A_REVERSE);
                mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", shared_mem->game_msg.omok_board[row][column]);
                attroff(A_REVERSE);
		
		// 사용자의 입력을 받음
		int c;

		c = getch();
		
		// 사용자가 방향키를 각각 입력했을 
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
		
		// 사용자가 엔터를 입력했을 때
		if(c == 10 || c == ' '){
			// player1의 턴이고 돌이 놓이지 않았을 때
			if(shared_mem->game_msg.omok_board[row][column] == '+'){
				// 서버에 자기가 놓은 위치와 턴 종료했음을 알림
				p(semid);
				shared_mem->game_msg.omok_board[row][column] = 'O';
				shared_mem->game_msg.turn_end = 1;
				shared_mem->game_msg.row = row;
				shared_mem->game_msg.col = column;
				clock_gettime(CLOCK_MONOTONIC, &shared_mem->start);
				v(semid);
				
				printOmokBoard();

				// 서버가 결과를 판단할 동안 대기
				usleep(100000);
			
				// 승리 했을 경우 승리 메세지 출력 후 종료
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
	initNcurses();		// ncurses 초기화

	// 선택 메뉴
	char* select[] = {"Game Start", "Exit"};

	// (8, 4)부터 시작
	int xStart = 8, yStart = 4;
	int highlight = 0;
	int c, i;

	// 메뉴 윈도우 생성
	WINDOW *menu_win;
	menu_win = newwin(6, 21, yStart,  xStart);
	box(menu_win, 0, 0);

	// 키보드 입력 허용
	keypad(menu_win, TRUE);
	
	// 메뉴 윈도우의 내용 입력
	wattron(menu_win, A_BOLD);
	mvwprintw(menu_win, 0, 6, "Omok Game");
	wattroff(menu_win, A_BOLD);
	
	// 화면 새로고침
	refresh();
	wrefresh(menu_win);

	while(1){
		// 화면 새로고침
		refresh();
		wrefresh(menu_win);

		// 사용자가 선택한 메뉴 선택
		for(i = 0; i < 2; i++){
			if(highlight == i)
				wattron(menu_win, A_REVERSE);
			
			mvwprintw(menu_win, 2 + i, 6, "%s", select[i]);
			wattroff(menu_win, A_REVERSE);
		}

		// 사용자의 입력 받음
		c = wgetch(menu_win);

		// 사용자의 키보드 입력에 따라 표시
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
		// 사용자가 엔터를 눌렀으면 무한루프 종료
		if(c == 10 || c == ' ') break;
	}
	
	// 선택한 메뉴가 Game Start면 대기실로 이동
	if(highlight == 0)
        	waitingRoom();
        
	// 선택한 메뉴가 Exit이면 프로그램 종료
	else{
        	endwin();
        	return;
	}


}


