#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>
#define MAX 1000
std::vector<int> c_sockets;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;


struct Targs {
    int c_socket;
    bool echo;
    bool broadcast;
    bool use; 
} args[MAX];

void* func(void* arg) {
    Targs* arguments = (Targs*)arg;
    int c_socket = arguments->c_socket;
    bool echo = arguments->echo;
    bool broadcast = arguments->broadcast;

    char buf[1000] = {0,};
    while (true) {
        int len = recv(c_socket, buf, sizeof(buf), 0);
        if (len <= 0) break;
        std::cout<<buf<<std::endl;
        if (echo) {
            send(c_socket, buf, len, 0);
        }
        if (broadcast) {
            pthread_mutex_lock(&mymutex);
            for (int sock : c_sockets) {
                if (sock != c_socket) {
                    if (send(sock, buf, len, 0) <= 0) {
                        std::cerr << "Error broadcasting\n";
                    }
                }
            }
            pthread_mutex_unlock(&mymutex);
        }

        memset(buf, 0, sizeof(buf));
    }

    close(c_socket);
    //remove c_socket 
    pthread_mutex_lock(&mymutex);
    c_sockets.erase(std::remove(c_sockets.begin(), c_sockets.end(), c_socket), c_sockets.end());
    pthread_mutex_unlock(&mymutex);

    arguments->use = false;
    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [-e[-b]]\n";
        return 1;
    }



    int port = atoi(argv[1]);
    bool echo =false;
    bool broadcast=false;


    for (int i=2; i<argc; i++) {
        std::string arg = argv[i];
        if (arg == "-e") {
            echo = true;
        } else if (arg == "-b") {
            broadcast = true;
        } else if ((arg == "-eb")||(arg == "-be")) {
            echo = true;
            broadcast = true;
        }
    }

    int s_socket= socket(AF_INET, SOCK_STREAM, 0);
    if (s_socket==0) {
        std::cerr << "Socket failed\n";
        return 1;
    }

    struct sockaddr_in addr;
    int optval = 1;
    if (setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
        std::cerr<< "setsockopt\n";
        return 1;
    }
    addr.sin_addr.s_addr =INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind(s_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr<< "bind error\n";
        return 1;
    }
    if (listen(s_socket, 5) < 0) {
        std::cerr<< "listen error\n";
        return 1;
    }
    for (int i = 0; i < MAX; ++i) {
        args[i].use=false;
    }
    while (true) {
        int len = sizeof(addr);
        int socket = accept(s_socket, (struct sockaddr *)&addr, (socklen_t*)&len);
        if (socket < 0) {
            std::cerr << "accept\n";
            continue;
        }

        int index;
        for (index = 0; index < MAX; index++) {
            if (!args[index].use) {
                args[index] = {socket, echo, broadcast, true};
                break;
            }
        }
        if (index == MAX) {
            std::cerr << "too many client is connected. sorry\n";
            close(socket);
            continue;
        }
        pthread_mutex_lock(&mymutex);
        c_sockets.push_back(socket);
        pthread_mutex_unlock(&mymutex);
        pthread_t thd;
        if (pthread_create(&thd, NULL, func, (void*)&args[index]) != 0) {
            std::cerr << "pthread_create error\n";
            args[index].use = false;
            close(socket);
            continue;
        }
        pthread_detach(thd);
    }

    return 0;
}