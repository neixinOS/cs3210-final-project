#include <inc/lib.h>
#include <inc/x86.h>

char board[3][3];
void init_board(void);
char check(void);
void player_move(void);
void computer_move(void);
void display_board(void);
int check_available(int row, int column);
void init_board(void)
{
  board[0][0] = '1';
  board[0][1] = '2';
  board[0][2] = '3';
  board[1][0] = '4';
  board[1][1] = '5';
  board[1][2] = '6';
  board[2][0] = '7';
  board[2][1] = '8';
  board[2][2] = '9';
}
void display_board(void)
{
  int i;

  for (i = 0; i < 3; i++) {
    cprintf(" %c | %c | %c ", board[i][0], board[i][1], board[i][2]);
    if (i != 2) {
      cprintf("\n---|---|---\n");
    }
  }
  cprintf("\n");
}

void player_move(void)
{
  int x;
  char *buf;
  int row, col;
  cprintf("Enter your move: ");
  int read;
  read = 1;
  while (read == 1) {
    buf = readline("Enter your move: ");
    if (buf != NULL) {
      read = 0;
    }
  }
  //scanf("%d", &x);
  x = *buf - '0';
  if ((x < 1) || (x > 9)) {
    cprintf("Invalid move, try again.\n");
    player_move();
  } else {

  if (x <= 3) {
    row = 0;
    col = x - 1;
  } else if (x <= 6) {
    row = 1;
    col = x - 4;
  } else {
    row = 2;
    col = x - 7;
  }
  
  if(check_available(row, col) < 0){
    cprintf("Invalid move, try again.\n");
    player_move();
  }else {
    board[row][col] = 'X';
  }
 }
}

void computer_move(void)
{
  int i, j;
  // check row by row, col by col, set the first valiable spot
  for (i = 0; i < 3; i++){
    for (j = 0; j < 3; j++) {
      if (check_available(i, j) > 0) {
        break;
      }
    }
    if (check_available(i, j) > 0) {
      break;
    }
  }
  // set O for computer_move
  board[i][j] = 'O';
}

int check_available(int row, int column)
{
  int check = board[row][column] - '0';
  if ((check >= 1) && (check <= 9))
    return 1;
  return -1;

}


char check(void)
{
  int i;

  for (i = 0; i < 3; i++) {
    if ((board[i][0] == board[i][1]) && (board[i][0] == board[i][2])) {
      return board[i][0];
    }
  }

  for (i = 0; i < 3; i++) {
    if ((board[0][i] == board[1][i]) && (board[0][i] == board[2][i])) {
      return board[0][i];
    }
  }

  for (i = 0; i < 3; i++) {
    if ((board[0][0] == board[1][1]) && (board[0][0] == board[2][2])) {
      return board[1][1];
    }
  }

  for (i = 0; i < 3; i++) {
    if ((board[0][2] == board[1][1]) && (board[2][0] == board[0][2])) {
      return board[1][1];
    }
  }

  return 'C';
}


void
umain(int argc, char **argv)
{
  // check countinue (cont = 'C')/win (cont = 'X')/lose (cont = 'O')
  char cont;
  cont = 'C';
  // cprintf("Single player Tic-Tac-Toe\n");
  cprintf("%m%s\n", 0x0400, "red ttt");
  init_board();
  while (cont == 'C') {
    // display playboard
    display_board();
    // get user input
    player_move();
    // check continute or not
    cont = check(); 
    if (cont != 'C') {
      // win/lose
      break;
    }
    // set computer move
    computer_move();
    // update continue
    cont = check(); /* see if winner */
  }

  if (cont == 'X') {
    cprintf("oh no\n");
    cprintf("%m%s\n", 0x0200, "You win!");
    //cprintf("You win!\n");
  } else {
    cprintf("You lose!\n");
  }
  display_board();
  breakpoint();
  //init_board();
  
}
