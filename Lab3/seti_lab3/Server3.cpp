#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

#include <pthread.h>
#include <vector>
#include <algorithm>

#define BUFFSIZE 81

pthread_mutex_t mutex; // ����������������� ����������

struct thread { // ��������� ������
    bool isActive = false;
    pthread_t pthread;
};

FILE* file;
std::vector<thread> threads;
thread* lastThread = nullptr; // ��������� �����
int lastSock = 0;

void* handler(void* args) // ����������
{
    thread* thisThread = lastThread;
    int sockNum = lastSock;
    int messageLength;
    char buff[BUFFSIZE];

    while (1)
    {
        messageLength = recv(sockNum, buff, BUFFSIZE, 0);
        if (messageLength <= 0) break;
        buff[messageLength] = '\0'; // 
        pthread_mutex_lock(&mutex); // ��� ���������� ������� ����� ��������� �������� ��������� � ����, ����� ��� ������ �� ������������ ������������

        file = fopen("data.txt", "a+");

        std::cout << "| Message: " << buff << std::endl;
        std::cout << "| Thread " << gettid() << std::endl;
        std::cout << "| Sock " << sockNum << std::endl << std::endl;

        fprintf(file, "message is: %s, thread: %d\n", buff, gettid());

        fclose(file);

        pthread_mutex_unlock(&mutex); // �������������
    }
    (*thisThread).isActive = false;
}


int main(int argc, char* argv[])
{
    int listener;
    socklen_t length = sizeof(struct sockaddr);
    struct sockaddr_in serverAddr;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = 0;

    if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) // ����������
	{
        std::cout << "Failed to contact" << std::endl;
        return -1;
    }

    listen(listener, 5); // 5 ��������� ����
    if (getsockname(listener, (struct sockaddr*)&serverAddr, &length)) {
        std::cout << "Error: socket name not received" << std::endl;
        return -1;
    }

    std::cout << "Server port: " << ntohs(serverAddr.sin_port) << std::endl;
    std::cout << "Server IP: " << inet_ntoa(serverAddr.sin_addr) << std::endl << std::endl;

    pthread_mutex_init(&mutex, 0); //�������������� ����������, �������� ���������� mutex

    pthread_attr_t attributes; // ������� ������� �����, ����������� ��� � ������������� ����� � ��������� � ��������� �����
    pthread_attr_init(&attributes); // ������� ������ ���������� ��������
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED); // ��������� ��� ������������ API ��� ��������

    for (int i = 0; i < 5; i++) { // ����� ��� ��������
        thread tmp; // �������  �����
        threads.push_back(tmp);
    }

    while (1) {
        int ssocket = accept(listener, 0, 0); // ��������� ���������� � ��������
        lastSock = ssocket;
        if (ssocket < 0) { 
            std::cout << "Error" << std::endl;
            continue; }

        auto freeThread = std::find_if(threads.begin(), threads.end(), [&](const thread& th)
		{return th.isActive == false; }); // ������� ��������� ���� ��� �������
        if (freeThread == threads.end()) continue; // ���� ��� ��������� ���� ������
        lastThread = &(*freeThread);

        if (pthread_create(&(*freeThread).pthread, &attributes, handler, (void*)&ssocket) < 0) // ������� ����� ������� �������� || � ������.
            std::cout << "Error thread creation" << std::endl;
        else (*freeThread).isActive = true;
    }
    close(listener);

    return 0;
}
