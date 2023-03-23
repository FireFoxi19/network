#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void sigchld_handler(int s)
{
  printf("[++++] Handling zombie process...\n");
  int sig;
  while(wait3(&sig, WNOHANG, (struct rusage *)0) >= 0);
}

int main ()
{
  int sockfd, ret;
  struct sockaddr_in serverAddr;

  int newSocket;
  struct sockaddr_in newAddr;

  socklen_t addr_size;
  //Создание буфера
  char buffer[1024];
  //Обозначает идентификацию процесса и используется для представления идентификаторов процессов. 
  pid_t childpid;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    printf("[ERROR] Error in connection. \n");
    exit(1);
  }
  printf("[++++] Server socket is created. \n");
  memset(&serverAddr, '\0', sizeof(serverAddr));
  bzero((char *) &serverAddr, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(0);
  // serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  newAddr.sin_family = AF_INET;
  ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
  if (ret < 0)
  {
    printf("[ERROR] Error in binding. \n");
    exit(1);
  }
  if (listen(sockfd, 10) == 0)
  {
    printf("[++++] Listening for incomming connections\n");
  } 
  else 
  {
    printf("[ERROR] Error in binding. \n");
  }

  socklen_t len = sizeof(struct sockaddr);
  if (getsockname(sockfd, (struct sockaddr *)&serverAddr, &len) < 0)
  {
    printf("[ERROR] in radom port assign\n");
    exit(1);
  }
  else
  {
    printf("[++++] Random port assigned\n");
    printf("[++++] Listening on %s:%d.\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
  }
  signal(SIGCHLD, sigchld_handler);
  for (;;)
  {
    newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
    if (newSocket < 0){
      printf("[ERROR] Cannot create socket");
    }
    socklen_t len = sizeof(struct sockaddr);
    getpeername(newSocket, (struct sockaddr*)&newAddr, &len);
    printf("[++++] Connection accepted from %s:%d.\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
    if ((childpid = fork()) == 0)
    {
      close(sockfd);

      for (;;)
      {
        if (recv(newSocket, buffer, 1024, 0) == 0)
        {
          printf("[++++] Client disconnected\n");
          break;
        }
        printf("[++++] Client: %s\n", buffer);
        send(newSocket, buffer, strlen(buffer), 0);
        bzero(buffer, sizeof(buffer));
      }
      close(newSocket);
      exit(0);
    }
  }

}
