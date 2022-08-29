#include "peer-socket.h"
#include <unistd.h>
#include <cerrno>
#include <iostream>

peer_socket::peer_socket(std::string ip, short port){
    peer.ip = ip;
    peer.port = port;
    peer.addr.sin_family = AF_INET;
    peer.addr.sin_addr.s_addr = inet_addr(ip.c_str());
    peer.addr.sin_port = htons(port);
}

peer_socket::~peer_socket(){
    close_connection();
    if(connection_th.joinable()){
        term = true;
        connection_th.join();
    }
}

void peer_socket::set_server(std::string server_ip, short server_port){
    mode = true;
    server.ip = server_ip;
    server.port = server_port;
    server.addr.sin_family = AF_INET;
    server.addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server.addr.sin_port = htons(server_port);
}

void peer_socket::set_client(){
    mode = false;
}

int peer_socket::wait_connection(){
    if(fd == 0){
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this](){ return fd > 0;});
        return 0;
    }else{
        return 0;
    }
}

int peer_socket::run(){

}

int peer_socket::stop(){

}

bool peer_socket::is_connected(){

}

int peer_socket::send_data(const std::vector<char>& buffer){
    
}

int peer_socket::recv_data(std::vector<char>& buffer){

}

int peer_socket::try_connection(){
    int tmp_fd{0};
    unsigned int addr_size = sizeof(sockaddr_in);
    if(mode){
        //server
        sockaddr_in peer_addr;
        tmp_fd = accept(server.fd, (sockaddr*)&peer_addr, (socklen_t*)&addr_size);
        if(tmp_fd < 0){
            if(errno == EWOULDBLOCK){
                return -1;
            }else{
                return -2;
            }
        }else{
            //seccess
            fd = tmp_fd;
        }
    }else{
        //client
        if(connect(fd, (sockaddr*)&peer.addr, addr_size) < 0){
            fd = 0;
            return -1;
        }
    }
    return 0;
}

int peer_socket::try_connection_loop(){
    while(!term){
        std::lock_guard<std::mutex> lock(mtx);
        int rc{0};
        rc = try_connection();
        if(rc == 0){
            //success
            cond.notify_all();
            return 0;
        }else if(rc == -2){
            std::cout << "socket error" << std::endl; 
            return -1;
        }
        sleep(1);
    }
    return 0;
}

int peer_socket::try_connection_start(){
    std::thread th(&peer_socket::try_connection_loop, this);
    connection_th = std::move(th);
    return 0;
}

int peer_socket::close_connection(){
    if(fd > 0){
        close(fd);
        fd = 0;
    }
}

