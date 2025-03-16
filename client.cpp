#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <csignal>
#include <netdb.h>

using namespace std;

bool stop_client = false;

void handle_termination_signal(int signal);
void parse_arguments(int argc, char *argv[], string &server_host, int &server_port, bool &run_infinite, int &packet_count);
int setup_connection(const string &server_host, int server_port, struct sockaddr_in &server_address);
void communicate_with_server(int &socket_fd, const string &server_host, int server_port, bool run_infinite, int packet_count);

int main(int argc, char *argv[]) {
    string server_host = "127.0.0.1";
    int server_port = 8080;
    int packet_count = 4;
    bool run_infinite = false;

    signal(SIGINT, handle_termination_signal);

    parse_arguments(argc, argv, server_host, server_port, run_infinite, packet_count);

    struct sockaddr_in server_address{};
    int socket_fd = -1;

    communicate_with_server(socket_fd, server_host, server_port, run_infinite, packet_count);

    if (socket_fd != -1) {
        close(socket_fd);
    }

    return 0;
}

void handle_termination_signal(int signal) {
    if (signal == SIGINT) {
        cout << "\033[33m" << "\nКлиент завершает работу по запросу пользователя." << "\033[0m" << endl;
        stop_client = true;
    }
}

void parse_arguments(int argc, char *argv[], string &server_host, int &server_port, bool &run_infinite, int &packet_count) {
    for (int i = 1; i < argc; ++i) {
        string argument = argv[i];
        if (argument == "-n" && i + 1 < argc) {
            packet_count = stoi(argv[++i]);
            cout << "\033[33m" << "Установлено количество пакетов: " << "\033[0m" << packet_count << endl;
        } else if (argument == "-i") {
            run_infinite = true;
            cout << "\033[33m" << "Включён бесконечный режим." << "\033[0m" << endl;
        } else if (argument.find(":") != string::npos) {
            server_host = argument.substr(0, argument.find(":"));
            server_port = stoi(argument.substr(argument.find(":") + 1));
        } else {
            cerr << "Использование: ./client <host>:<port> [-n <packets>] или [-i]" << endl;
            exit(EXIT_FAILURE);
        }
    }
}

int setup_connection(const string &server_host, int server_port, struct sockaddr_in &server_address) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        cerr << "\033[31m" << "Ошибка создания сокета!" << "\033[0m" << endl;
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    struct hostent *he = gethostbyname(server_host.c_str());
    if (he == nullptr) {
        cerr << "\033[31m" << "Ошибка: не удалось разрешить имя хоста!" << "\033[0m" << endl;
        close(socket_fd);
        return -1;
    }
    memcpy(&server_address.sin_addr, he->h_addr_list[0], he->h_length);

    return socket_fd;
}

void communicate_with_server(int &socket_fd, const string &server_host, int server_port, bool run_infinite, int packet_count) {
    struct sockaddr_in server_address{};
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    string ping_message = "Пиф";
    int sent_packets = 0;

    while (!stop_client && (run_infinite || sent_packets < packet_count)) {
        if (socket_fd == -1) {
            socket_fd = setup_connection(server_host, server_port, server_address);
            if (socket_fd < 0) {
                cerr << "\033[31m" << "Не удалось подключиться к серверу! Повторная попытка через 3 секунды..." << "\033[0m" << endl;
                sleep(3);
                continue;
            }
            int connection_status = connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
            if (connection_status == -1) {
                cerr << "\033[31m" << "Сервер недоступен. Повторная попытка через 3 секунды..." << "\033[0m" << endl;
                close(socket_fd);
                socket_fd = -1;
                sleep(3);
                continue;
            }
            cout << "\033[32m" << "Подключение к серверу успешно установлено." << "\033[0m" << endl;
        }

        ssize_t bytes_sent = send(socket_fd, ping_message.c_str(), ping_message.size(), 0);
        if (bytes_sent < 0) {
            cerr << "\033[31m" << "Ошибка отправки сообщения! Переустановка соединения..." << "\033[0m" << endl;
            close(socket_fd);
            socket_fd = -1;
            continue;
        }
        cout << "Отправлено: " << "\033[35m" << ping_message << "\033[0m" << endl;

        ssize_t bytes_received = read(socket_fd, buffer, BUFFER_SIZE);
        if (bytes_received > 0) {
            string response(buffer, bytes_received);
            cout << "Получено: " << "\033[34m" << response << "\033[0m" << endl;

            if (response != "Паф") {
                cout << "\033[31m" << "Неизвестное сообщение от сервера: " << response << " — завершение соединения." << "\033[0m" << endl;
                close(socket_fd);
                socket_fd = -1;
                continue;
            }
        } else if (bytes_received == 0) {
            cout << "\033[33m" << "Сервер завершил соединение." << "\033[0m" << endl;
            close(socket_fd);
            socket_fd = -1;
            continue;
        } else {
            cerr << "\033[31m" << "Ошибка получения сообщения от сервера! Переустановка соединения..." << "\033[0m" << endl;
            close(socket_fd);
            socket_fd = -1;
            continue;
        }

        ++sent_packets;
        sleep(1);
    }

    if (socket_fd != -1) {
        close(socket_fd);
    }
}
