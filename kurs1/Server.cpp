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
    std::string Name = ""; // имя пользователя
    int Descriptor; // дескриптор
};

std::vector<Client> Clients; // вектор в котором будут храниться объекты клиентов
std::string filename = "Clients.txt";
std::fstream ClientsFile; // чтение и запись в текстовик

void reaper(int sig) { // ф-ия остановки дочерних процессов когда они завершили работу
    int status;
    while(wait3(&status, WNOHANG, (struct rusage*)0) >= 0);
}

void RefreshClients(){ // ф-ия обновления списка пользователей
    std::string name;
    int descriptor;
    auto clientsCopy = Clients; // копирует список текущих пользователей
    Clients.clear(); // очищает список

    ClientsFile.clear(); // очищает чтение и запись
    ClientsFile.seekg(0); 

    while(ClientsFile >> name >> descriptor){ // перемещает указатель на начало и считывает данные из файла
        Clients.push_back(Client{name, descriptor});
    }

    for(const auto& oldClient : clientsCopy){ // затем проходит по списку clientsCopy 
        if(std::find_if(Clients.begin(), Clients.end(), [&](const Client& client) 
		{return client.Name == oldClient.Name;}) == Clients.end())
		{ 
            close(oldClient.Descriptor); // закрывает сокеты клиентов которые были удалены из списка Clients
        } 
    }
}

void UpdateClientsFile(){ // ф-ия которая открывает и закрывает файл в режиме записи и перезаписывает в него список пользователей
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
    int clientSocket; //сокет клиента
    int listener; // слушатель
    int messageLength; // длина сообщения
    char buff[BUFFSIZE]; // буфер для сообщений
    socklen_t length = sizeof(struct sockaddr);
    struct sockaddr_in serverAddr;

    listener = socket(AF_INET, SOCK_STREAM, 0); // создание сокета
    serverAddr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&serverAddr.sin_addr);
    serverAddr.sin_port = 0;

    if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) { // привязка к сокету
        printf("Failed binding\n");
        exit(1);
    }

    listen(listener, 5); // ожидание входящих соединений
    if (getsockname(listener, (struct sockaddr*)&serverAddr, &length)) {
        printf("Error when calling getsockname\n");
        exit(1);
    }

    printf("Server: Port Number - %d\n", ntohs(serverAddr.sin_port));
    printf("SERVER: IP %s\n", inet_ntoa(serverAddr.sin_addr));

    signal(SIGCHLD, reaper); // установка обработчика сигнала
    #pragma endregion Init

    int clientCount = 3; // кол-во клиентов

    #pragma region Connecting
    for(int i = 0; i < clientCount; i++){
        clientSocket = accept(listener, 0, 0); // ожидание входящего соединения

        messageLength = recv(clientSocket, buff, BUFFSIZE, 0); // принятия сообщения от клиента recv
        if(messageLength == -1) std::cout << "Recieve error" << std::endl;

        buff[messageLength] = '\0'; // добавляется 0 символ в конец полученного сообщения чтобы преобразовать его в строку
        std::string message(buff); // создается строка из полученного сообщения
        std::cout << "Request message: " << message << std::endl;

        auto sameName = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client){return client.Name == message;});
        if(sameName != Clients.end()){
            std::string request("Server: This username already exist.\n"); // проверка введения никнейма на повтор
            if(send(clientSocket, request.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;
            close(clientSocket);
            i--;
        }

        Clients.push_back(Client{message, clientSocket});
        message = "Server: Connected.";
        if(send(clientSocket, message.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl;
    }

    for(const auto& user : Clients){ // цикл по всем клиентам которые находятся в векторе клиентов
        std::string availableClients = ""; // пустая строка для хранения инф-ии о доступных клиентах
        int availableClientsCount = Clients.size() - 1; // кол-во доступных клиентов которые могут общаться с данным пользователем
        availableClients += "Server: " + std::to_string(availableClientsCount) + " available users:\n"; // строка в которой инф-ия о доступных клиентах

        int clientNum = 1; // задается номер клиента
        for(const auto& client : Clients){ // получения списка всех доступных клиентов для данного пользователя
            if(client.Name  != user.Name){ // имя текущего клиента != имени текущего пользователя
                availableClients += std::to_string(clientNum) + ". " + client.Name + "\n"; // строка в которой номер клиента и его имя чтобы добавить его в список доступных клиентов
                clientNum++; // счетчик клиентов
            }
        }

        availableClients += "Server: Chat Started.\n";
        if(send(user.Descriptor, availableClients.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // инфа о доступных клиентах
        // если отправка сообщений не удалась выводится сообщение об ошибке
    }

    UpdateClientsFile();
    #pragma endregion Connecting
// ЦИКЛ ПО ВСЕМ КЛИЕНТАМ
    for(int i = 0; i < clientCount; i++) {
        clientSocket = Clients[i].Descriptor; // сокет для текущего клиента
        std::string clientName = Clients[i].Name; // получается имя текущего клиента
        switch(fork()){ // запускается новый процесс для каждого клиента который подключен к серверу
            case -1:{
                printf("Error in fork\n");
                exit(1);
            }
            case 0:{ // код для дочернего процесса который обрабатывает сообщения от клиента
                close(listener); // закрытие сокета для прослушивания новых подключений, тк этот процесс обрабатывает сообщения от клиента

                while (1) // бесконечный цикл обработки сообщений от клиента
                {
                    messageLength = recv(clientSocket, buff, BUFFSIZE, 0);
                    if(messageLength == -1) std::cout << "Recieve error" << std::endl; // получение сообщения не удалось

                    buff[messageLength] = '\0'; // 0 символ в конец полученного сообещния чтобы преобразовать его в строку
                    std::string message(buff); // создается строка из полученного сообщения
                    std::cout << "Request message: " << message << std::endl;

                    if(1){
                        if(message == "disconnect"){ // команда отключения
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

                        if(message.find("EraseClient") != std::string::npos){ // скрывает сообщения от клиента чье имя указано после ирейза
                            std::string deleteName(message.substr(message.find(" ") + 1)); 
                            
                            std::cout << deleteName << std::endl; // выводим имя клиента которое нужно удалить 
                            auto deleteClient = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client)
							{return client.Name == deleteName;}); // находим клиента в списке у которого имя совпадает с deleteName
                            Clients.erase(deleteClient); // удаляем найденного клиента, удаляем элемент из контейнера
                            continue;
                        }

                        std::string messageForReciever = clientName + ": " + message; // создаем сообщение которое отправим остальным клиентам

                        for(const auto& client : Clients){ // перебираем всех клиентов в списке
                            if(client.Name != clientName){ // проверка что это не отправитель сообщения, чтобы самому себе не скинуть
                                std::cout << "Reciever: " << client.Name << " Answer: " << messageForReciever << std::endl << std::endl;// кому отправлено и что содержит
                                if(send(client.Descriptor, messageForReciever.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // если не удалось ошибка
                            }
                        }
                    }
                }
                
                auto sender = std::find_if(Clients.begin(), Clients.end(), [&](const Client& client)
				{return client.Name == clientName;});// находим отправителя в списке 
                Clients.erase(sender); // удаляем отправителя из списка
                UpdateClientsFile(); // обновляем текстовик

                close(clientSocket); // закрываем сокет 
                exit(0);
            }
            default:{
                //close(clientSocket);
                while(waitpid(-1, NULL, WNOHANG) > 0); // завершение дочернего процесса, сборка мусора используя флаг WNOHANG
            }
        }
    }
    wait(NULL);
    close(listener);

    return 0;
}
