#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
  int clientSocket, ret;
  struct sockaddr_in serverAddr;
  char buffer[1024];

  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  
  if (clientSocket < 0)
  {
    printf("[ERROR] Error in connection. \n");
    exit(1);
  }

  printf("[iiii] Client socket is created. \n");
  int port = 0;
  printf("[iiii] Input port...\n");
  scanf("%d", &port);
  char ip[32];
  printf("[iiii] Input ip...\n");
  scanf("%s", ip);
  printf("[++++] IP = %s PORT = %d\n", ip, port);
  memset(&serverAddr, '\0', sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(ip);

  ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
  if (ret < 0) 
  {
    printf("[ERROR] Error in connection.");
    exit(1);
  }

  printf("[++++] Connected to server. \n");

  for (;;) 
  {
    int i, n;
    printf("[iiii] Input i...\n");
    scanf("%d", &i);
    sprintf(buffer, "%d", i);
    printf("[iiii] Input n-times to print i...\n");
    scanf("%d", &n);

    for (int a = 0; a < n; a++)
    {
      printf("[++++] Sending i...\n");
      sleep(i);
      send(clientSocket, buffer, strlen(buffer), 0);
    }
    close(clientSocket);
    exit(1);
  }
}
