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

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#define BUFFSIZE 1024

struct Client{
    std::string Name = ""; // ��� ������������
    int Descriptor; // ����������
};

std::vector<Client> Clients; // ������ � ������� ����� ��������� ������� ��������
std::string filename = "Clients.txt";
std::fstream ClientsFile; // ������ � ������ � ���������

void reaper(int sig) { // �-�� ��������� �������� ��������� ����� ��� ��������� ������
    int status;
    while(wait3(&status, WNOHANG, (struct rusage*)0) >= 0);
}

void RefreshClients(){ // �-�� ���������� ������ �������������
    std::string name;
    int descriptor;
    auto clientsCopy = Clients; // �������� ������ ������� �������������
    Clients.clear(); // ������� ������

    ClientsFile.clear(); // ������� ������ � ������
    ClientsFile.seekg(0); 

    while(ClientsFile >> name >> descriptor){ // ���������� ��������� �� ������ � ��������� ������ �� �����
        Clients.push_back(Client{name, descriptor});
    }

    for(const auto& oldClient : clientsCopy){ // ����� �������� �� ������ clientsCopy 
        if(std::find_if(Clients.begin(), Clients.end(), [&](const Client& client) 
		{return client.Name == oldClient.Name;}) == Clients.end())
		{ 
            close(oldClient.Descriptor); // ��������� ������ �������� ������� ���� ������� �� ������ Clients
        } 
    }
}

void UpdateClientsFile(){ // �-�� ������� ��������� � ��������� ���� � ������ ������ � �������������� � ���� ������ �������������
    ClientsFile.close();
    ClientsFile.open("Clients.txt", std::fstream::out | std::fstream::in);

    for(const auto& client : Clients){
        ClientsFile << client.Name << " " << client.Descriptor << std::endl;
    }
}

int main(int argc, char* argv[])
{
    #pragma region Init
    ClientsFile.open(filename.c_str(), std::fstream::out | std::fstream::in | std::fstream::trunc);
    int clientSocket; //����� �������
    int listener; // ���������
    int messageLength; // ����� ���������
    char buff[BUFFSIZE]; // ����� ��� ���������
    socklen_t length = sizeof(struct sockaddr);
    struct sockaddr_in serverAddr;

    listener = socket(AF_INET, SOCK_STREAM, 0); // �������� ������
    serverAddr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&serverAddr.sin_addr);
    serverAddr.sin_port = 0;

    if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) { // �������� � ������
        printf("Failed binding\n");
        exit(1);
    }

    listen(listener, 5); // �������� �������� ����������
    if (getsockname(listener, (struct sockaddr*)&serverAddr, &length)) {
        printf("Error when calling getsockname\n");
        exit(1);
    }

    printf("Server: Port Number - %d\n", ntohs(serverAddr.sin_port));
    printf("SERVER: IP %s\n", inet_ntoa(serverAddr.sin_addr));

    signal(SIGCHLD, reaper); // ��������� ����������� �������
    #pragma endregion Init

    int clientCount = 3; // ���-�� ��������

    #pragma region Connecting
    for(int i = 0; i < clientCount; i++){
        clientSocket = accept(listener, 0, 0); // �������� ��������� ����������

        messageLength = recv(clientSocket, buff, BUFFSIZE, 0); // �������� ��������� �� ������� recv
        if(messageLength == -1) std::cout << "Recieve error" << std::endl;

        buff[messageLength] = '\0'; // ����������� 0 ������ � ����� ����������� ��������� ����� ������������� ��� � ������
        std::string message(buff); // ��������� ������ �� ����������� ���������
        std::cout << "Request message: " << message << std::endl;

        auto sameName = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client){return client.Name == message;});
        if(sameName != Clients.end()){
            std::string request("Server: This username already exist.\n"); // �������� �������� �������� �� ������
            if(send(clientSocket, request.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;
            close(clientSocket);
            i--;
        }

        Clients.push_back(Client{message, clientSocket});
        message = "Server: Connected.";
        if(send(clientSocket, message.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;
    }

    for(const auto& user : Clients){ // ���� �� ���� �������� ������� ��������� � ������� ��������
        std::string availableClients = ""; // ������ ������ ��� �������� ���-�� � ��������� ��������
        int availableClientsCount = Clients.size() - 1; // ���-�� ��������� �������� ������� ����� �������� � ������ �������������
        availableClients += "Server: " + std::to_string(availableClientsCount) + " available users:\n"; // ������ � ������� ���-�� � ��������� ��������

        int clientNum = 1; // �������� ����� �������
        for(const auto& client : Clients){ // ��������� ������ ���� ��������� �������� ��� ������� ������������
            if(client.Name  != user.Name){ // ��� �������� ������� != ����� �������� ������������
                availableClients += std::to_string(clientNum) + ". " + client.Name + "\n"; // ������ � ������� ����� ������� � ��� ��� ����� �������� ��� � ������ ��������� ��������
                clientNum++; // ������� ��������
            }
        }

        availableClients += "Server: Chat Started.\n";
        if(send(user.Descriptor, availableClients.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // ���� � ��������� ��������
        // ���� �������� ��������� �� ������� ��������� ��������� �� ������
    }

    UpdateClientsFile();
    #pragma endregion Connecting
// ���� �� ���� ��������
    for(int i = 0; i < clientCount; i++) {
        clientSocket = Clients[i].Descriptor; // ����� ��� �������� �������
        std::string clientName = Clients[i].Name; // ���������� ��� �������� �������
        switch(fork()){ // ����������� ����� ������� ��� ������� ������� ������� ��������� � �������
            case -1:{
                printf("Error in fork\n");
                exit(1);
            }
            case 0:{ // ��� ��� ��������� �������� ������� ������������ ��������� �� �������
                close(listener); // �������� ������ ��� ������������� ����� �����������, �� ���� ������� ������������ ��������� �� �������

                while (1) // ����������� ���� ��������� ��������� �� �������
                {
                    messageLength = recv(clientSocket, buff, BUFFSIZE, 0);
                    if(messageLength == -1) std::cout << "Recieve error" << std::endl; // ��������� ��������� �� �������

                    buff[messageLength] = '\0'; // 0 ������ � ����� ����������� ��������� ����� ������������� ��� � ������
                    std::string message(buff); // ��������� ������ �� ����������� ���������
                    std::cout << "Request message: " << message << std::endl;

                    if(1){
                        if(message == "disconnect"){ // ������� ����������
                            std::string info("Server: Chat closed.\n");
                            if(send(clientSocket, info.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;

                            info = std::string(clientName + " left chat.");
                            for(const auto& client : Clients){
                                if(client.Name != clientName){
                                    std::cout << "Reciever: " << client.Name << " Answer: " << info << std::endl << std::endl;
                                    if(send(client.Descriptor, info.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;
                                }
                            }
                            break;
                        }

                        if(message.find("EraseClient") != std::string::npos){ // �������� ��������� �� ������� ��� ��� ������� ����� ������
                            std::string deleteName(message.substr(message.find(" ") + 1)); 
                            
                            std::cout << deleteName << std::endl; // ������� ��� ������� ������� ����� ������� 
                            auto deleteClient = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client)
							{return client.Name == deleteName;}); // ������� ������� � ������ � �������� ��� ��������� � deleteName
                            Clients.erase(deleteClient); // ������� ���������� �������, ������� ������� �� ����������
                            continue;
                        }

                        std::string messageForReciever = clientName + ": " + message; // ������� ��������� ������� �������� ��������� ��������

                        for(const auto& client : Clients){ // ���������� ���� �������� � ������
                            if(client.Name != clientName){ // �������� ��� ��� �� ����������� ���������, ����� ������ ���� �� �������
                                std::cout << "Reciever: " << client.Name << " Answer: " << messageForReciever << std::endl << std::endl;// ���� ���������� � ��� ��������
                                if(send(client.Descriptor, messageForReciever.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // ���� �� ������� ������
                            }
                        }
                    }
                }
                
                auto sender = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client)
				{return client.Name == clientName;});// ������� ����������� � ������ 
                Clients.erase(sender); // ������� ����������� �� ������
                UpdateClientsFile(); // ��������� ���������

                close(clientSocket); // ��������� ����� 
                exit(0);
            }
            default:{
                //close(clientSocket);
                while(waitpid(-1, NULL, WNOHANG) > 0); // ���������� ��������� ��������, ������ ������ ��������� ���� WNOHANG
            }
        }
    }
    wait(NULL);
    close(listener);

    return 0;
}
