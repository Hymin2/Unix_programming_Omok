#include <ncurses.h>
#include <pthread.h>
#include <wchar.h>

#define MAX_ROW 15
#define MAX_COLUMN 15

void initMenu();
void initNcurses();
void gameRoom();
void waitingRoom();

int main(){
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

void waitingRoom(){
	initNcurses();

	int i, highlight = 0;
	int xStart = 5, yStart = 3;

	char* select[] = {"Ready!!", "Exit"};
	char* status[] = {"Wait", "Join", "Ready"};

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
	mvwprintw(player1_status, 0, 0, "%s", status[2]);

	WINDOW* player2_status = newwin(1, 7, yStart + 2, xStart + 20);
        mvwprintw(player2_status, 0, 0, "%s", status[2]);

        WINDOW* ready_exit_box = newwin(3, 30, yStart + 5, xStart);
        box(ready_exit_box, 0, 0);

        keypad(ready_exit_box, TRUE);

        refresh();
	
        wrefresh(player1);
	wrefresh(player1_status);
        wrefresh(player2);
	wrefresh(player2_status);
        wrefresh(ready_exit_box);

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

                if(c == 10 || c == ' '){
			touchwin(ready_exit_box);
			wrefresh(ready_exit_box);
			refresh();
			select[0] = "Test";
			mvwprintw(ready_exit_box, 1, 5, "%s", select[0]);
			mvwprintw(ready_exit_box, 1, 20, "%s", select[1]);
			touchwin(ready_exit_box);
                        wrefresh(ready_exit_box);
                        refresh();
			wgetch(ready_exit_box);
			
		}
        }

	endwin();

	if(highlight == 0) gameRoom();
	else initMenu();
}

void gameRoom(){
	initNcurses();
	keypad(stdscr,TRUE);

	char* status[] = {"wait", "turn", "win!", "lose"};
	char omok_board[MAX_ROW][MAX_COLUMN];
	
	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int myTurn = 1;
	int row = 0, column = 0;

	for(i = 0; i < 15; i++){
		for(k = 0; k < 15; k++)
			omok_board[i][k] = '+';
	}
	move(yStart , xStart + 2);

	while(1){
		for(i = 0; i < 15; i++){
			mvprintw(i + yStart, xStart, "|");
			for(k = 0; k < 15; k++){
				mvprintw(i + yStart,  1 + xPoint * k + xStart, "-%c-", omok_board[i][k]);
			}
			mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
		}

		
		attron(A_REVERSE);
		mvprintw(yStart + row, xStart + column * xPoint + 2, "%c", omok_board[row][column]);
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

		}

		if(c == 10 || c == ' '){
			if(myTurn == 1 && omok_board[row][column] == '+')
				omok_board[row][column] = 'O';
		}

		refresh();

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

		if(c == 10 || c == ' ')
			break;
	}

	endwin();

	if(highlight == 0)
		waitingRoom();
	else return;
}

