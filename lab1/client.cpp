#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)
#include <cstdlib>
#define _BSD_SOURCE
#define BUFLEN 81
int main(int argc, char *argv[]) {
    int sock;
    for (int i = 0; i < 10; i++) {
        struct sockaddr_in servAddr, clientAddr;
        struct hostent *hp;
        if (argc < 4) {
            printf(" Ввести udpclient имя_хоста порт сообщение\n");
            exit(1);
        }
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("He могу получить socket\n");
            exit(1);
        }
        bzero((char *)&servAddr, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        hp = gethostbyname(argv[1]);
        bcopy(hp->h_addr, &servAddr.sin_addr, hp->h_length);
        servAddr.sin_port = htons(atoi(argv[2]));
        bzero((char *)&clientAddr, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        clientAddr.sin_port = 0;
        int time = atoi(argv[3]);
        if (bind(sock, (struct sockaddr *)&clientAddr, sizeof(clientAddr))) {
            perror("Клиент не получил порт");
            exit(1);
        }
        printf("CLIENT: готов к пересылке.\n");
        if (sendto(sock, argv[3], strlen(argv[3]), 0,
                   (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            perror("Проблемы с sendto . \n");
            exit(1);
        }
        printf("CLIENT: Пересылка завершена. Счастливо оставаться. \n");
        char buf[BUFLEN];
        socklen_t length = sizeof(clientAddr);
        int msglength;
        bzero(buf, sizeof(BUFLEN));
        if ((msglength = recvfrom(sock, buf, BUFLEN, 0,
            (struct sockaddr *)&clientAddr, &length)) <0)
		{
            perror("Плохой socket клиента.");
            exit(1);
        }
        printf("SERVER: Сообщение:\n%s\n\n", buf);
        sleep(time);
    }
    close(sock);
}
