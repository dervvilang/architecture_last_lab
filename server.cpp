#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <csignal>

using namespace std;

int server_socket;
int client_socket = -1;

void handle_signal(int signal);
void initialize_server_socket(int port);
void accept_client_connection();
void process_client_messages();
void close_sockets();

int main(int argc, char *argv[]) {
    if (argc != 3 || string(argv[1]) != "-p") {
        cerr << "Ввод: ./server -p <port>" << endl;
        return EXIT_FAILURE;
    }
    const int port = stoi(argv[2]);
    signal(SIGINT, handle_signal);

    initialize_server_socket(port);

    cout << "\033[32m" << "Сервер запущен на порту " << port << "." << "\033[0m" << endl;

    while (true) {
        accept_client_connection();
        process_client_messages();
    }

    return 0;
}

void handle_signal(int signal) {
    if (signal != SIGINT) return;
    cout << "\033[33m" << "\nЗакрытие сервера пользователем." << "\033[0m" << endl;
    close_sockets();
    exit(0);
}

void initialize_server_socket(int port) {
    struct sockaddr_in server_addr{};

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "\033[31m" << "Ошибка при создании сокета!" << "\033[0m"  << endl;
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "\033[31m" << "Ошибка привязки сокета к порту!" << "\033[0m" << endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 2) < 0) {
        cerr << "\033[31m" << "Ошибка при переходе в режим прослушивания!" << "\033[0m" << endl;
        close(server_socket);
        exit(EXIT_FAILURE);
    }
}

void accept_client_connection() {
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    if (client_socket == -1) {
        cout << "\033[33m" << "Сервер ожидает подключение." << "\033[0m" << endl;
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            cerr << "\033[31m" << "Ошибка при принятии подключения клиента!" << "\033[0m" << endl;
            return;
        }
        cout << "\033[32m" << "Подключился клиент: " << inet_ntoa(client_addr.sin_addr)
             << ":" << ntohs(client_addr.sin_port) << "\033[0m" << endl;
    }
}

void process_client_messages() {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    string pong = "Паф";

    while (client_socket != -1) {
        ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            string message(buffer, bytes_read);
            cout << "Получено: " << "\033[35m" << message << "\033[0m" << endl;

            if (message == "Пиф") {
                ssize_t bytes_sent = send(client_socket, pong.c_str(), pong.size(), 0);
                if (bytes_sent < 0) {
                    cerr << "\033[31m" << "Ошибка при отправке данных клиенту!" << "\033[0m" << endl;
                    break;
                }
                cout << "Отправлено: " << "\033[34m" << pong << "\033[0m" << endl;
            } else {
                cout << "Неизвестное сообщение: " << message << " — закрываем соединение с клиентом." << endl;
                close(client_socket);
                client_socket = -1;
                break;
            }
        } else if (bytes_read == 0) {
            cout << "\033[33m" << "Клиент завершил соединение." << "\033[0m" << endl;
            close(client_socket);
            client_socket = -1;
            break;
        } else {
            cerr << "\033[31m" << "Ошибка при чтении данных от клиента!" << "\033[0m" << endl;
            close(client_socket);
            client_socket = -1;
            break;
        }
    }
}

void close_sockets() {
    if (client_socket != -1)
        close(client_socket);
    close(server_socket);
}
