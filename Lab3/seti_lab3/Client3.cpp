#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <iostream>

#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int sock;
    char buff[1024];

    if (argc < 4) {
        std::cout << "Wrong input: should be 'ip adress, port, message" << std::endl;
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Can't get socket" << std::endl;
        return -1;
    }

    struct sockaddr_in addres;

    addres.sin_family = AF_INET;
    addres.sin_port = htons(atoi(argv[2]));
    addres.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&addres, sizeof(addres)) < 0) {
        std::cout << "Error in connect" << std::endl;
        return -1;
    }

    std::cout << "| Client: Port number - " << ntohs(addres.sin_port) << std::endl;

    char message[10];
    for (int i = 0; i < atoi(argv[3]); i++)
	{
        std::cout << message[i] << std::endl;
        send(sock, argv[4], sizeof(argv[4]), 0);
        sleep(atoi(argv[3]));
    }

    close(sock);
    return 0;
}
