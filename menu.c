#include <ncurses.h>
#include <pthread.h>
#include <wchar.h>

int initMenu();
void initNcurses();
void createOmokBoard();

int main(){
	int ret;
	ret = initMenu();
	
	createOmokBoard();
}

void initNcurses(){
	initscr();
	clear();
	noecho();
	cbreak();
	curs_set(0);
}

void createOmokBoard(){
	initNcurses();
	keypad(stdscr,TRUE);

	char* turn[] = {"Your turn", "Other's turn"};
	char omok_board[15][15];
	
	int xMax, yMax;
	int i, k, xPoint = 3;
	int myTurn = 1;
	int row = 0, column = 0;

	getmaxyx(stdscr, yMax, xMax);

	for(i = 0; i < 15; i++){
		for(k = 0; k < 15; k++)
			omok_board[i][k] = '+';
	}
	
	move(yMax / 5, xMax / 5 + 2);

	while(1){
		for(i = 0; i < 15; i++){
			mvprintw(i + yMax / 5, xMax / 5, "|");
			for(k = 0; k < 15; k++){
				mvprintw(i + yMax / 5,  1 + xPoint * k + xMax / 5, "-%c-", omok_board[i][k]);
			}
			mvprintw(i + yMax / 5, 1 + xPoint * 15 + xMax / 5, "|");
		}

		
		attron(A_REVERSE);
		mvprintw(yMax / 5 + row, xMax / 5 + column * xPoint + 2, "%c", omok_board[row][column]);
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

int initMenu(){
	initNcurses();

	char* select[] = {"Game Start", "Exit"};

	int xMax, yMax;
	int highlight = 1;
	int c, i;

	getmaxyx(stdscr, yMax, xMax);

	WINDOW *menu_win;
	
	menu_win = newwin(yMax / 4, xMax / 4, yMax / 4,  xMax / 4);

	box(menu_win, 0, 0);

	keypad(menu_win, TRUE);
	
	wattron(menu_win, A_BOLD);
	mvwprintw(menu_win, 0, xMax / 8 - 4.5, "Omok Game");
	wattroff(menu_win, A_BOLD);
	refresh();
	wrefresh(menu_win);

	while(1){
		for(i = 0; i < 2; i++){
			if(highlight == i + 1)
				wattron(menu_win, A_REVERSE);
			
			mvwprintw(menu_win, yMax / 8 - 2 + i, xMax / 8 - 4.5, "%s", select[i]);
			wattroff(menu_win, A_REVERSE);
		}

		c = wgetch(menu_win);

		switch(c){
			case KEY_UP:
				if(highlight == 1) highlight = 2;
				highlight--;
				break;
			case KEY_DOWN:
				if(highlight == 2) highlight = 1;
				highlight++;
				break;
			default:
				break;
		}	

		if(c == 10 || c == ' ')
			break;
	}

	endwin();

	return highlight;
}

