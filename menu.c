#include <ncurses.h>
#include <pthread.h>

void initMenu();

int main(){
	initMenu();
	
}

int initMenu(){
	initscr();
	clear();
	noecho();
	cbreak();
	curs_set(0);

	char* select[] = {"Game Start", "Exit"};

	int xMax, yMax;
	int highlight = 1;
	int c, i;

	getmaxyx(stdscr, yMax, xMax);

	WINDOW *menu_win;
	
	menu_win = newwin(yMax / 4, xMax / 4, yMax / 3,  xMax / 3);

	box(menu_win, 0, 0);

	keypad(menu_win, TRUE);
	
	wattron(menu_win, A_BOLD);
	mvwprintw(menu_win, 0, 6, "Omok Game");
	wattroff(menu_win, A_BOLD);
	refresh();
	wrefresh(menu_win);

	while(1){
		for(i = 0; i < 2; i++){
			if(highlight == i + 1)
				wattron(menu_win, A_REVERSE);
			
			mvwprintw(menu_win, i + 2, 6, "%s", select[i]);
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

		if(c == 10)
			break;
	}

	endwin();

	return highlight;
}

