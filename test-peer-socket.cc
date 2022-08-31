#include "peer-socket.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv){
    if(argc < 2){
        cout << "./test-peer-socket [client/server]" << endl;
        return 0;
    }

    string mode(argv[1]);
    peer_socket psocket;

    const string ip = "127.0.0.1";
    const short port1 = 8080;
    const short port2 = 8081;
    const char* send_buffer = "hello";
    char recv_buffer[7];

    if(mode == "client"){
        cout << "client" << endl;
        psocket.setup(peer_socket::CLIENT, ip, port1, ip, port2);

        if(psocket.connect_peer() < 0){
            cout << "connect_peer error" << endl;
            return 0;
        }

        if(psocket.send_data(send_buffer, 6) < 0){
            cout << "psocket send fail" << endl;
            return 0;
        }else{
            cout << send_buffer << endl;
        }

        if(psocket.disconnect_peer() < 0){
            cout << "psocket disconnect fail" << endl;
            return 0;
        }

        if(psocket.connect_peer() < 0){
            cout << "connect_peer error" << endl;
            return 0;
        }

        if(psocket.recv_data(recv_buffer, 6) < 0){
            cout << "psocket recv fail" << endl;
            return 0;
        }else{
            cout << recv_buffer << endl;
        }

        if(psocket.disconnect_peer() < 0){
            cout << "psocket disconnect fail" << endl;
            return 0;
        }
    }else if(mode == "server"){
        cout << "server" << endl;
        psocket.setup(peer_socket::SERVER, ip, port2, ip, port1);

        if(psocket.start_listen() < 0){
            cout << "psocket start listen error" << endl;
            return 0;
        }

        if(psocket.connect_peer() < 0){
            cout << "connect_peer error" << endl;
            return 0;
        }

        if(psocket.recv_data(recv_buffer, 6) < 0){
            cout << "psocket recv fail" << endl;
            return 0;
        }else{
            cout << recv_buffer << endl;
        }

        if(psocket.disconnect_peer() < 0){
            cout << "psocket disconnect fail" << endl;
            return 0;
        }

        if(psocket.connect_peer() < 0){
            cout << "connect_peer error" << endl;
            return 0;
        }

        if(psocket.send_data(send_buffer, 6) < 0){
            cout << "psocket send fail" << endl;
            return 0;
        }else{
            cout << send_buffer << endl;
        }

        if(psocket.disconnect_peer() < 0){
            cout << "psocket disconnect fail" << endl;
            return 0;
        }

        if(psocket.stop_listen() < 0){
            cout << "psocket stop listen error" << endl;
            return 0;
        }
    }
    return 0;
}
