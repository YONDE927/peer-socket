#include "peer-socket.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <system_error>
#include <iostream>

peer_socket::peer_socket(){
}

peer_socket::~peer_socket(){
    term = true;
    close_peer();
    close_self();
    stop_listen();
}

void peer_socket::setup(mode_t m, std::string peer_ip, short peer_port, std::string self_ip, short self_port){
    std::lock_guard<std::mutex> lock(mtx);
    if(m == UNSET){
        std::cout << "args mode_t m cannot be UNSET" << std::endl;
        return;
    }
    mode = m;
    peer.ip = peer_ip;
    peer.port = peer_port;
    peer.addr.sin_family = AF_INET;
    peer.addr.sin_addr.s_addr = inet_addr(peer_ip.c_str());
    peer.addr.sin_port = htons(peer_port);
    self.ip = self_ip;
    self.port = self_port;
    self.addr.sin_family = AF_INET;
    self.addr.sin_addr.s_addr = inet_addr(self_ip.c_str());
    self.addr.sin_port = htons(self_port);
}

void peer_socket::start_connecting(){
    {
        std::lock_guard<std::mutex> lock(mtx);
        //check if setup
        if(mode == UNSET){
            std::cout << "socket mode is UNSET" << std::endl;
            return;
        }
    }
    try_connect_entry();
    wait_connect();
}

int peer_socket::close_peer(){
    std::lock_guard<std::mutex> lock(mtx);
    if(fd > 0){
        close(fd);
    }
    fd = 0;
    status = DISCONNECTED;
    return 0;
}

int peer_socket::close_self(){
    std::lock_guard<std::mutex> lock(mtx);
    if(mode == SERVER){
        if(self.listen_fd > 0){
            close(self.listen_fd);
        }
    }
    self.listen_fd = 0;
    status = DISCONNECTED;
    return 0;
}

int peer_socket::start_listen(){
    std::lock_guard<std::mutex> lock(mtx);
    if(mode == SERVER){
        self.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(self.listen_fd < 0){
            return -1;
        }

        //reuse addr
        int yes{1};
        if(setsockopt(self.listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0){
            return -1;
        }

        //non-blocking
        ioctl(self.listen_fd, FIONBIO, &yes);

        //ポート接続
        if(bind(self.listen_fd, (struct sockaddr*)&self.addr, sizeof(sockaddr_in)) < 0){
            return -1;
        }

        //接続待ち
        if(listen(self.listen_fd, 5) < 0){
            return -1;
        }
    }else{
        std::cout << "socket mode is not SERVER" << std::endl;
    }
    return 0;
}


int peer_socket::stop_listen(){
    std::lock_guard<std::mutex> lock(mtx);
    if(mode == SERVER){
        if(self.listen_fd != 0){
            close(self.listen_fd);
            self.listen_fd = 0;
        }
    }else{
        std::cout << "socket mode is not SERVER" << std::endl;
    }
    return 0;
}

bool peer_socket::is_connected(){
    std::lock_guard<std::mutex> lock(mtx);
    return status == CONNECTED; 
}

void peer_socket::wait_connect(){
    std::unique_lock<std::mutex> lock(mtx);
    cond.wait(lock, [this](){return status == CONNECTED;});
}

int peer_socket::send_data(const void* buffer, int size){
    std::lock_guard<std::mutex> lock(mtx);
    int rc{0};
    if(status == CONNECTED){
        rc = send(fd, buffer, size, 0);
        if(rc == size){
            //send success
            return rc;
        }else{
            close(fd);
            fd = 0;
            //DISCONNECTEDへ状態遷移
            status = DISCONNECTED;        
        }
    }
    return -1;
}

int peer_socket::recv_data(void* buffer, int size){
    //接続しているならば、送る
    {
        std::lock_guard<std::mutex> lock(mtx);
        int rc{0};
        if(status == CONNECTED){
            std::lock_guard<std::mutex> lock(mtx);
            rc = recv(fd, buffer, size, MSG_WAITALL);
            if(rc == size){
                //recv success
                return rc;
            }else{
                close(fd);
                fd = 0;
                //DISCONNECTEDへ状態遷移
                status = DISCONNECTED;        
            }
        }
    }
    //非接続ならば再接続を開始する
    if(status == DISCONNECTED){
        try_connect_entry();
    }
    return -1;
}

void peer_socket::try_connect_entry(){
    std::lock_guard<std::mutex> lock(mtx);
    if(mode == UNSET){
        std::cout << "socket mode is UNSET" << std::endl;
        return;
    }
    if(status == DISCONNECTED){
        status = CONNECTING;
        std::thread th(&peer_socket::try_connect, this);
        th.detach();
    }else if(status == CONNECTED){
        std::cout << "socket is connected" << std::endl;
    }else if(status == CONNECTING){
        std::cout << "connection_thread is already running" << std::endl;
    }
    return;
}

int peer_socket::try_connect_client(){
    socklen_t socklen(sizeof(sockaddr_in));
    while(!term){
        int tmp_socket = socket(AF_INET, SOCK_STREAM, 0);
        {
            std::lock_guard<std::mutex> lock(mtx);
            if(connect(tmp_socket, (sockaddr*)&peer.addr, socklen) < 0){
                std::error_code ec(errno, std::generic_category());
                std::cout << "connecting attempt fail: " << ec.message() << std::endl; 
                close(tmp_socket);
            }else{
                std::cout << "connected" << std::endl;
                //acceptされたことの確認・メッセージ
                char sign{0};
                recv(tmp_socket, &sign, sizeof(char), 0);
                if(sign == 0){
                    std::cout << "connected peer but rejected" << std::endl; 
                    close(tmp_socket);
                }else{
                    fd = tmp_socket;
                    //CONNECTEDへ状態遷移
                    status = CONNECTED;
                    cond.notify_all();
                    break;
                }
            }
        }
    }
    return 0;
}

int peer_socket::try_connect_server(){
    socklen_t socklen(sizeof(sockaddr_in));
    sockaddr_in tmp_addr;
    while(!term){
        {
            std::lock_guard<std::mutex> lock(mtx);
            int tmp_socket = accept(self.listen_fd, (sockaddr*)&tmp_addr, &socklen);
            if(tmp_socket < 0){
                if(errno == EWOULDBLOCK){
                    std::cout << "accept attempt fail..." << std::endl;
                }else{
                    std::error_code ec(errno, std::generic_category());
                    std::cout << "socket error happen: " << ec.message() << std::endl; 
                }
            }else{
                //set blocking
                int no{0};
                ioctl(tmp_socket, FIONBIO, &no);
                //check if valid peer addr
                if(tmp_addr.sin_addr.s_addr == peer.addr.sin_addr.s_addr &&
                        tmp_addr.sin_port == peer.addr.sin_port){
                    char sign = 1;
                    if(send(tmp_socket, &sign, sizeof(char), 0) != 1){
                        std::cout << "connected valid peer but send error" << std::endl; 
                        close(tmp_socket);
                    }else{
                        fd = tmp_socket;
                        //CONNECTEDへ状態遷移
                        status = CONNECTED;
                        cond.notify_all();
                        break;
                    }
                }else{
                    char sign = 0;
                    send(tmp_socket, &sign, sizeof(char), 0);
                    close(tmp_socket);
                    std::cout << "invalid peer" << std::endl; 
                }
            }
        }
        sleep(TRY_INTERVAL);
    }
    return 0;
}

int peer_socket::try_connect(){
    if(mode == CLIENT){
        return try_connect_client();
    }else if(mode == SERVER){
        return try_connect_server();
    }else if(mode == UNSET){
        std::cout << "こんなことはありえない..." << std::endl;
        return -1; 
    }
    return 0;
}
