#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


void* recv_msg(void* socket) {
    int c_sock = *(int*)socket;
    while (true) {
        char buf[1000] = {0,};
        int len = recv(c_sock, buf, sizeof(buf), 0);
        if (len <= 0) {
            std::cout << "recv error\n";
            break;
        }
        std::cout << buf << "\n";
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    int c_sock = 0;
    struct sockaddr_in server_addr;

    if ((c_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket error\n";
        return 1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
        std::cerr << "inet_pton error\n";
        return 1;
    }

    if (connect(c_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "connect error \n";
        return 1;
    }
    std::cout << "Connected\n";
    std::cout << "If you want to exit, enter 'exit'.\n";
    pthread_t thr;
    if (pthread_create(&thr, NULL, recv_msg, (void*)&c_sock) != 0) {
        std::cerr << "pthread_create error\n";
        return 1;
    }
    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        if (msg == "exit") {
            break;
        }
        send(c_sock, msg.c_str(), msg.size(), 0);
    }
    pthread_join(thr, NULL);
    close(c_sock);
    return 0;
}
