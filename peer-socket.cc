#include "peer-socket.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <system_error>
#include <iostream>

int peer_socket::connect_client(){
    const int yes{1};
    socklen_t socklen(sizeof(sockaddr_in));
    int tmp_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(setsockopt(tmp_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0){
        return -1;
    }
    if(bind(tmp_socket, (struct sockaddr*)&self.addr, sizeof(sockaddr_in)) < 0){
        return -1;
    }
    while(!term){
        if(connect(tmp_socket, (sockaddr*)&peer.addr, socklen) < 0){
            std::error_code ec(errno, std::generic_category());
            std::cout << "connecting attempt fail: " << ec.message() << std::endl; 
            sleep(TRY_INTERVAL);
            continue;
        }else{
            //acceptされたことの確認・メッセージ
            char sign{0};
            if(recv(tmp_socket, &sign, sizeof(char), 0) != sizeof(char)){
                std::cout << "接続されたが確認パケットを受け取る前にエラーが起きた" << std::endl; 
                close(tmp_socket);
                sleep(TRY_INTERVAL);
                continue;
            }
            if(sign == 0){
                std::cout << "connected peer but rejected" << std::endl; 
                close(tmp_socket);
                return -1;
            }else{
                fd = tmp_socket;
                //CONNECTEDへ状態遷移
                status = CONNECTED;
                return 0;
            }
        }
    }
    return 0;
}

int peer_socket::connect_server(){
    socklen_t socklen(sizeof(sockaddr_in));
    sockaddr_in tmp_addr;
    while(!term){
        int tmp_socket = accept(listen_fd, (sockaddr*)&tmp_addr, &socklen);
        if(tmp_socket < 0){
            if(errno == EWOULDBLOCK){
                std::cout << "accept attempt fail..." << std::endl;
                sleep(TRY_INTERVAL);
                continue;
            }else{
                std::error_code ec(errno, std::generic_category());
                std::cout << "socket error happen: " << ec.message() << std::endl; 
                return -1;
            }
        }else{
            //set blocking
            int no{0};
            ioctl(tmp_socket, FIONBIO, &no);
            //check if valid peer addr
            if(tmp_addr.sin_addr.s_addr == peer.addr.sin_addr.s_addr &&
                tmp_addr.sin_port == peer.addr.sin_port){
                char sign = 1;
                if(send(tmp_socket, &sign, sizeof(char), 0) != sizeof(char)){
                    std::cout << "接続されたが確認パケットを送信時にエラーが起きた" << std::endl; 
                    close(tmp_socket);
                    sleep(TRY_INTERVAL);
                    continue;
                }else{
                    fd = tmp_socket;
                    //CONNECTEDへ状態遷移
                    status = CONNECTED;
                    return 0;
                }
            }else{
                char sign = 0;
                std::cout << "invalid peer" << std::endl; 
                if(send(tmp_socket, &sign, sizeof(char), 0) != sizeof(char)){
                    std::cout << "接続されたが確認パケットを送信時にエラーが起きた" << std::endl; 
                }
                close(tmp_socket);
                sleep(TRY_INTERVAL);
                continue;
            }
        }
    }
    return 0;
}

peer_socket::peer_socket(){
}

peer_socket::~peer_socket(){
}

void peer_socket::setup(mode_t m, std::string peer_ip, short peer_port, std::string self_ip, short self_port){
    if(m == UNSET){
        std::cout << "args mode_t m cannot be UNSET" << std::endl;
        return;
    }
    if(mode != UNSET){
        std::cout << "cannot change socket mode, deconstruct plz" << std::endl;
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

int peer_socket::start_listen(){
    if(mode != SERVER){
        std::cout << "mode is not SERVER" << std::endl;
        return -1;
    }
    if(status == CONNECTED){
        std::cout << "It's connected yet, close peer first" << std::endl;
        return -1;
    }else if(status == CONNECTING){
        std::cout << "It's connecting yet, close peer first" << std::endl;
        return -1;
    }else{
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_fd < 0){
            return -1;
        }
        int yes{1};
        if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0){
            return -1;
        }
        ioctl(listen_fd, FIONBIO, &yes);
        if(bind(listen_fd, (struct sockaddr*)&self.addr, sizeof(sockaddr_in)) < 0){
            return -1;
        }
        if(listen(listen_fd, 5) < 0){
            return -1;
        }
        std::cout << "listening start" << std::endl;
    }
    return 0;
}

int peer_socket::stop_listen(){
    if(mode != SERVER){
        std::cout << "mode is not SERVER" << std::endl;
        return -1;
    }
    if(status == CONNECTED){
        std::cout << "It's connected yet, close peer first" << std::endl;
        return -1;
    }else if(status == CONNECTING){
        std::cout << "It's connecting yet, close peer first" << std::endl;
        return -1;
    }else{
        if(listen_fd > 0){
            close(listen_fd);
            listen_fd = 0;
            std::cout << "listening stop" << std::endl;
        }
        return 0;
    }
}

bool peer_socket::is_connected(){
    return status == CONNECTED; 
}

int peer_socket::connect_peer(){
    if(status == CONNECTED){
        std::cout << "It's connected yet, use this socket" << std::endl;
        return -1;
    }else if(status == CONNECTING){
        std::cout << "It's connecting yet, wait connecting task" << std::endl;
        return -1;
    }else{
        //CONNECTINGへ状態遷移
        status = CONNECTING;
        if(mode == CLIENT){
            if(connect_client() < 0){
                std::cout << "connect_client error" << std::endl;
                return -1;
            }else{
                std::cout << "connected" << std::endl;
                return 0;
            }
        }else if(mode == SERVER){
            if(connect_server() < 0){
                std::cout << "connect_server error" << std::endl;
                return -1;
            }else{
                std::cout << "connected" << std::endl;
                return 0;
            }
        }else{
            std::cout << "論外" << std::endl;
            return -1;
        }
    }
}

int peer_socket::disconnect_peer(){
    if(status == CONNECTED){
        close(fd);
        status = DISCONNECTED;
        std::cout << "disconnected" << std::endl;
        return 0;
    }else if(status == CONNECTING){
        std::cout << "It's connecting yet, wait connecting task" << std::endl;
        return -1;
    }else{
        std::cout << "already disconnected" << std::endl;
        return 0;
    }
}

int peer_socket::send_data(const void* buffer, int size){
    int rc{0};
    if(status == CONNECTED){
        rc = send(fd, buffer, size, 0);
        if(rc == size){
            //send success
            return rc;
        }else{
            //DISCONNECTEDへ状態遷移
            close(fd);
            status = DISCONNECTED;
            return -1;
        }
    }else{
        std::cout << "connection is offline..." << std::endl;
        return -1;
    }
}

int peer_socket::recv_data(void* buffer, int size){
    //接続しているならば、送る
    int rc{0};
    if(status == CONNECTED){
        rc = recv(fd, buffer, size, MSG_WAITALL);
        if(rc == size){
            //recv success
            return rc;
        }else{
            //DISCONNECTEDへ状態遷移
            close(fd);
            status = DISCONNECTED;
            return -1;
        }
    }else{
        std::cout << "connection is offline..." << std::endl;
        return -1;
    }
}
