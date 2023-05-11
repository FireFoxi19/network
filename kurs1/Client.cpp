#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <arpa/inet.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <iostream>

#define BUFFSIZE 1024

bool threadTerminated = false; // отслеживание состояния потока
bool needThreadTerminate = false; // необходимость завершения потока

void AsyncSend(int& sock){ // ф-ия отправки сообщения через сокет
    while(1){
        std::string message = ""; // инициализируется пустая строка
        std::cin >> message;

        if(needThreadTerminate){ // если переменная == тру то ф-ия AsyncSend завершается
            threadTerminated = true;
            return;
        }

        if(!message.empty()){ // отправка сообщения через сокет, если вернет -1 то выводит ошибку об отправке
            if(send(sock, message.c_str(), BUFFSIZE, 0) == -1){
                std::cout << "Send error" << std::endl;
            }
        }
        else{ // если сообщение пустое то выводит ошибку
            std::cout << "Message can't be empty, try again.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    #pragma region Init // ИНИЦИАЛИЗАЯ 
    int sock; // для хранения сокета
    char buff[BUFFSIZE]; // буфер принимаемых данных

    if (argc < 4) { // проверка ввода на кол-во аргументов адреса, порта и никнейма
        printf("Wrong input: should be 'ip address, port, name.'\n");
        exit(1);
    }

    std::string name(argv[3]); // строка name содержащая 3 аргумент - имя клиента
    if(name.empty()){ // проверка пустое имя или нет
        std::cout << "Name is empty.";
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // сообщение об ошибке получения сокета
        printf("Can't get socket\n");
        exit(1);
    }

    struct sockaddr_in addres; // структура где хранятся адреса и порта сервера

    addres.sin_family = AF_INET;
    addres.sin_port = htons(atoi(argv[2])); // устанавливается порт, atoi преобразующий строку в целое число, htons портв сетевой порядок байтов
    addres.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr преобразует строку с IP-адресом в двоичный формат

    if(connect(sock, (struct sockaddr*)&addres, sizeof(addres)) < 0){ // установка соединения с сервером, sock сокет и размер структуры addres
        printf("Error in connect\n"); // если меньше 0 то выводит ошибку
        exit(1);
    }

    printf("CLIENT: Port number - %d\n", ntohs(addres.sin_port));
    #pragma endregion Init

    #pragma region Connecting // ПОДКЛЮЧЕНИЕ К СЕРВЕРУ
    if(send(sock, name.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // отправляется имя клиента через сокет
    if(recv(sock, buff, BUFFSIZE, 0) == -1) std::cout << "Recieve error" << std::endl; // принимается сообщения с сервера в буфер

    std::string onNameAnswer(buff); // строка содержащая данные из буфера
    std::cout << onNameAnswer << std::endl; // вывод содержимого строки
    if(onNameAnswer.find("exist") != std::string::npos)
	{ // проверка на содержание подстроки exist, find возвращает позицию первого вхождения или std::string::npos если подстрока не найдена
        exit(1); // если подстрока найдена, программа завершается
    }

    if(recv(sock, buff, BUFFSIZE, 0) == -1) std::cout << "Recieve error" << std::endl; // принимаетс список пользователей с сервера в буфер
    std::string usersList(buff); // строка содержащая данные из буфера
    std::cout << usersList << std::endl;

    std::thread asyncReciever(AsyncSend, std::ref(sock)); // создается объект потока asyncReciever, который вызывает ф-ию AsyncSend передавая ей ссылку на сокет
    asyncReciever.detach(); // отсоединение потока asyncReciever от главного потока
    #pragma endregion Connecting

    while(1){
        int msgLen = recv(sock, buff, BUFFSIZE, 0); // принимается сообщение с сервер и его длинна сохраняется в msgLen
        if(msgLen == -1) std::cout << "Recieve error" << std::endl; // проверка на ошибку при приеме сообщения


        std::string answer(buff); // создается строка содержащая данные из буфера
        std::cout << answer << std::endl;
        if(answer.find("closed") != std::string::npos){ // проверка строки на содержание closed
            needThreadTerminate = true; // если closed найдена то тру
            break; // выход из цикла while 
        }

        if(answer.find("left chat.") != std::string::npos){ // проверка содержит ли строка answer подстроку left chat

            std::string message("EraseClient " + answer.substr(0, answer.find(" "))); // создается строка message содержащая подстроку EraseClient
            // и часть answer до первого пробела
            if(send(sock, message.c_str(), BUFFSIZE, 0) == -1) std::cout << "Send error" << std::endl; // отправка сообщения через сокет
        }
    }
    while(!threadTerminated){} // пустой цикл ожидающий завершения потока asyncReciever

    close(sock); // закрытие сокета
    return 0;
}
