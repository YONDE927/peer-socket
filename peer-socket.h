#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <arpa/inet.h>

struct peer_info{
    std::string ip;
    short port;
    sockaddr_in addr;
};

struct server_info : public peer_info{
    int fd;   
};

class peer_socket{
    private:
        bool mode{false};
        bool term{false};
        int fd{0};
        std::mutex mtx;
        std::thread connection_th;
        std::condition_variable cond;
        peer_info peer;
        server_info server;

        int try_connection();
        int try_connection_loop();
        int try_connection_start();
        int close_connection();
    public:
        peer_socket(std::string peer_ip, short peer_port);
        ~peer_socket();
        void set_server(std::string server_ip, short server_port);
        void set_client();
        int wait_connection();
        int run();
        int stop();
        bool is_connected();
        int send_data(const std::vector<char>& buffer);
        int recv_data(std::vector<char>& buffer);
};
