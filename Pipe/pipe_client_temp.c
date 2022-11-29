void gameRoom()
{
	initNcurses();
	int xStart = 5, yStart = 3;
	int i, k, xPoint = 3;
	int row = 0, column = 0;
	void*ret;

	move(yStart, xStart + 2);
	keypad(stdscr, TRUE);
	refresh();

	for (i = 0; i < ROW; i++) {
		for (k = 0; k < COLUMN; k++) {
			send_msg.g_msg.omok_board[i][k] = '+';
		}
	}

	while(1){
		printOmokBoard();
		pthread_create(&game_thread, NULL, checkGameRoomOpponentTurnEnd, NULL);
		pthread_join(game_thread, &ret);
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

		attron(A_REVERSE);
		mvprintw(row + yStart, xStart + column * xPoint + 2, "%c", omok_board[row][column]);
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
				send_msg.g_msg.col = col;
				write(write_fd, &msg, sizeof(msg));
			}
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
		}
	endwin();
}	

void printOmokBoard() {
	for (i = 0; i < ROW; i++) {
		mvprintw(i + yStart, xStart, "|");
		for (k = 0; k < COLUMN; k++) {
			mvprintw(i + yStart, 1 + xPoint * k + xStart, "-%c-", send_msg.g_msg.omok_board[i][k]);
		}
		mvprintw(i + yStart, 1 + xPoint * 15 + xStart, "|");
	}
	refresh();
}

void* checkGameRoomOpponentTurnEnd()
{
	int xPoint = 3, xStart = 5, yStart = 3;
	int i, k, ret = 0;

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
