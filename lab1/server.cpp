#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)
#include <cstdlib>
#define _BSD_SOURCE
#define BUFLEN 81
using namespace std;
template <typename T>
std::string toString(T val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}
main() {
    int sockMain, msgLength;
    socklen_t length;
    struct sockaddr_in servAddr, clientAddr;
    char buf[BUFLEN];
    if ((sockMain = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Сервер не может открыть socket для UDP.");
        exit(1);
    }
    // printf("%d\n", sockMain);
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = 0;

    if (bind(sockMain, (struct sockaddr *)&servAddr, sizeof(servAddr))) {
        perror("Связывание сервера неудачно.");
        exit(1);
    }
    length = sizeof(servAddr);
    if (getsockname(sockMain, (struct sockaddr *)&servAddr, &length)) {
        perror("Вызов getsockname неудачен.");
        exit(1);
    }
    printf("СЕРВЕР: номер порта - % d\n", ntohs(servAddr.sin_port));
    for (;;) {
        length = sizeof(clientAddr);
        int msglength;
        bzero(buf, sizeof(BUFLEN));
        if ((msglength = recvfrom(sockMain, buf, BUFLEN, 0,
            (struct sockaddr *)&clientAddr, &length)) < 0)
		{
            perror("Плохой socket клиента.");
            exit(1);
        }
        printf("SERVER: IP адрес клиента: %s\n",
            inet_ntoa(clientAddr.sin_addr)); //Функция inet_ntoa() преобразует IP-адрес in, заданный в сетевом порядке расположения байтов, 
			   	// в стандартный строчный вид, из номеров и точек.
        printf("SERVER: PORT клиента: %d\n", ntohs(clientAddr.sin_port));
        // printf("SERVER: Р”Р»РёРЅР° СЃРѕРѕР±С‰РµРЅРёСЏ - %d\n", msgLength);
        printf("SERVER: Сообщение: %s\n\n", buf);
        string str = "PORT: " + toString(ntohs(clientAddr.sin_port)) +
                     "\nIP:" + toString(inet_ntoa(clientAddr.sin_addr)) +
                     "\nMsg: " + toString(buf) + "\n";
        if (sendto(sockMain, str.c_str(), str.length() + 1, 0,
                   (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
            perror("Проблемы с sendto . \n");
            exit(1);
        }
    }
}
