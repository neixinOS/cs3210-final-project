#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#define PORT 7

#define BUFFSIZE 32
#define GAMEBOARDSIZE 128
#define MAXPENDING 5    // Max connection requests
const char* board[3][3];
bool avail[3][3];
const char* X="\033[0;31mX\033[0m";
const char* O="\033[0;34mO\033[0m";
char client_symbol;
static void
die(char *m)
{
  cprintf("%s\n", m);
  exit();
}


void init_board(void)
{
  board[0][0] = "1";
  board[0][1] = "2";
  board[0][2] = "3";
  board[1][0] = "4";
  board[1][1] = "5";
  board[1][2] = "6";
  board[2][0] = "7";
  board[2][1] = "8";
  board[2][2] = "9";
  int i, j;
  for (i= 0; i < 3; ++i)
    for (j = 0; j < 3; ++j)
      avail[i][j] = true;
}
int check_available(int row, int column)
{
  return avail[row][column] ? 1 : -1;
}
char check(void)
{
  int i;
  int j;
  int r;
  //check for the rows
  for (i = 0; i < 3; i++) {
    if ((board[i][0] == board[i][1]) && (board[i][0] == board[i][2])) {
      return board[i][0] == X ? 'X' : 'O';
    }
  }

  //check for the columns
  for (i = 0; i < 3; i++) {
    if ((board[0][i] == board[1][i]) && (board[0][i] == board[2][i])) {
      return board[0][i] == X ? 'X' : 'O';
    }
  }

  //check for the diagonal
  for (i = 0; i < 3; i++) {
    if ((board[0][0] == board[1][1]) && (board[0][0] == board[2][2])) {
      return board[1][1] == X ? 'X' : 'O';
    }
  }

  //check for diagonal
  for (i = 0; i < 3; i++) {
    if ((board[0][2] == board[1][1]) && (board[2][0] == board[0][2])) {
      return board[1][1] == X ? 'X' : 'O';
    }
  }
  for(i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      r = check_available(i,j);
      if (r == 1) {
        return 'C';
      }
    }
  }
  return 'T';
}

void
display_board()
{
  int i;
  
  for (i = 0; i < 3; i++) {
    fprintf(1," %s | %s | %s ", board[i][0], board[i][1], board[i][2]);
    cprintf(" %s | %s | %s ", board[i][0], board[i][1], board[i][2]);

    if (i != 2) {
      fprintf(1,"\n---|---|---\n");
      cprintf("\n---|---|---\n");
    }
  }
  fprintf(1,"\n");
  cprintf("\n");
}

// void
// display_client(int sock)
// {
//   // int i;
//   // char buffer[BUFFSIZE];
//   // for (i = 0; i < 3; i++) {
//   //   buffer = buffer + board[i][0] + ' | ' + board[i][1] + ' | ' + board[i][2] + ' ';
//   //   if (i != 2) {
//   //     buffer = buffer + "\n---|---|---\n";
//   //   }
//   // }
//   // buffer = buffer + "\n";
//   // return buffer;
//   int i;
//   char temp[GAMEBOARDSIZE];
//   char buff[GAMEBOARDSIZE];
//   char s = 'E';
//   char flag1[50] = " | ";
//   char *ret;
//   for (i = 0; i < 3; i++) {
//     snprintf(temp, GAMEBOARDSIZE, " %c%s%c%s%c ", board[i][0],flag1, board[i][1],flag1,board[i][2]);
//     strcat(buff,temp);
//     if (i != 2) {
//       strcat(buff,"\n---|---|---\n");
//     }
//   }
//   strcat(buff, "\n");

//   cprintf("%s", buff);
//   write(sock, buff, sizeof(buff));
//   //cprintf("%s", buffer);
//   // buffer = buffer + "\n";
// }


void client_move(const char* symbol, int sock)
{
  int x;
  int row, col;
  char buffer[BUFFSIZE];
  int received = -1;
  char msg[BUFFSIZE] = "Client: Enter your move: \n";
  //ask client to move
  write(sock, msg, sizeof(msg));
  int r;
  r = 1;
  while (r == 1) {
    if ((received = read(sock, buffer, BUFFSIZE)) < 0)
      die("Failed to receive initial bytes from client");
    else 
      r =0;
  }
  x = buffer[0] - '0';
  if ((x < 1) || (x > 9)) {
    cprintf("Invalid move, try again.\n");
    client_move(symbol, sock);
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
      client_move(symbol, sock);
    }else {
      board[row][col] = symbol;
      avail[row][col] = false;
    }
  }

}
void server_move(const char* symbol)
{
  int x;
  char *buf;
  int row, col;
  cprintf("Server: Enter your move: \n");
  int read;
  read = 1;
  while (read == 1) {
    buf = readline("Server: Enter your move: \n");
    if (buf != NULL) {
      read = 0;
    }
  }
  //scanf("%d", &x);
  x = *buf - '0';
  if ((x < 1) || (x > 9)) {
    cprintf("Invalid move, try again.\n");
    server_move(symbol);
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
      server_move(symbol);
    }else {
      board[row][col] = symbol;
      avail[row][col] = false;
    }
  }
}


void 
play_game(int sock) {
  char buffer[BUFFSIZE];
  int received = -1;
  char client_board[GAMEBOARDSIZE];
  char cont;
  cont = 'C';
  char msg1[BUFFSIZE] = "You win!\n";
  char msg2[BUFFSIZE] = "You lose!\n";
  //initialize board
  init_board();
  cprintf("Welcome to Tic Tac Toe\n");
  fprintf(1, "Welcome to Tic Tac Toe!\n");
  fprintf(1, "Enter the 'X' or 'O' \n");
  //receive client's symbol
  if ((received = read(sock, buffer, BUFFSIZE)) < 0)
    die("Failed to receive initial bytes from client");
  client_symbol = buffer[0];

  while(client_symbol != 'X' && client_symbol != 'O') {
    fprintf(1, "Invalid enter! Enter the 'X' or 'O' \n");
    if ((received = read(sock, buffer, BUFFSIZE)) < 0)
      die("Failed to receive initial bytes from client");
    client_symbol = buffer[0];
  }

  while(received > 0 && cont == 'C') {
    //display board on both side
		display_board();
    fprintf(1, "waiting for server player..... \n");
    //display_client(sock);
    //server go
    if (client_symbol == 'X' || client_symbol == 'x'){
      client_symbol = 'X';
      server_move(O);
    } else
      server_move(X);
    cont = check();
    if (cont!='C')
      break;
    //display board to both side
    display_board();
    //display_client(sock);
	  cprintf("waiting for client player.....\n");
    //client go
	  client_move(client_symbol == 'X' ? X : O, sock);
    //update continue
		cont = check(); /* see if winner */
	}

  int a[50];
  char *win = "Server: Win!\n";
  char *lose = "Server:Lose!\n";
  char *tie = "Server: TIE!\n";

  int j = 0;

  if (cont == 'T') {
    fprintf(1, "TIE!!!!\n");
    cprintf("TIE!!!!!\n");

    while (tie[j]) {
      a[j] = tie[j];
      j++;
    }
    j = 0;
    sys_raid_init(a);

  } else if (cont == client_symbol) {
		write(sock, msg1, BUFFSIZE);
		cprintf("You lose!\n");

    while (lose[j]) {
      a[j] = lose[j];
      j++;
    }
    j = 0;
    sys_raid_init(a);

  } else {
    cprintf("You Win!\n");

    while (win[j]) {
      a[j] = win[j];
      j++;
    }
    j = 0;
    cprintf("%d\n", sizeof(a) / sizeof(int));
    sys_raid_init(a);

		write(sock, msg2, BUFFSIZE);
  }

	display_board();
  //display_client(sock);

	close(sock);
}



void
umain(int argc, char **argv)
{
  int serversock, clientsock;
  struct sockaddr_in echoserver, echoclient;
  char buffer[BUFFSIZE];
  unsigned int echolen;
  int received = 0;
  char board[3][3];

  // Create the TCP socket
  if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    die("Failed to create socket");

  cprintf("opened socket\n");

  // Construct the server sockaddr_in structure
  memset(&echoserver, 0, sizeof(echoserver));             // Clear struct
  echoserver.sin_family = AF_INET;                        // Internet/IP
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY);         // IP address
  echoserver.sin_port = htons(PORT);                      // server port

  cprintf("trying to bind\n");

  // Bind the server socket
  if (bind(serversock, (struct sockaddr *)&echoserver,
           sizeof(echoserver)) < 0)
    die("Failed to bind the server socket");

  // Listen on the server socket
  if (listen(serversock, MAXPENDING) < 0)
    die("Failed to listen on server socket");

  cprintf("bound\n");

  // Run until canceled
  while (1) {
    unsigned int clientlen = sizeof(echoclient);
    // Wait for client connection
    if ((clientsock = accept(serversock, (struct sockaddr *)&echoclient,
                  &clientlen)) < 0)
      die("Failed to accept client connection");
    cprintf("Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
    play_game(clientsock);
  }
  close(serversock);
}
