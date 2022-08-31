//仕様
//peerはipとportのセット
//クライアントとサーバーを意識しないバックエンド(設定除く)
//接続が切れたかどうかはsend,recvで検知する
//
//再接続手順
//共通
//term変数を用いて再接続試行を止めることができる
//1秒間隔で試行する
//サーバー
//acceptはノンブロックに設定される
//クライアント
//connectはそのまま
//接続完了時
//
//終了手順
//ユーザー側に責任転嫁する。
//
//接続試行側がブロックされてしまう時にどう判別するか
//

#pragma once
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#define TRY_INTERVAL 3

struct peer_info{
    std::string ip;
    short port{0};
    sockaddr_in addr;
};

class peer_socket{
    private:
        enum status_t{
            CONNECTED,
            CONNECTING,
            DISCONNECTED
        }status{DISCONNECTED};
        bool term{false};
        int fd{0};
        int listen_fd{0};
        peer_info peer;
        peer_info self;
    public:
        enum mode_t{
            UNSET,
            CLIENT,
            SERVER
        }mode{UNSET};
    private:
        int connect_client();
        int connect_server();
    public:
        peer_socket();
        ~peer_socket();
        void setup(mode_t m, std::string peer_ip, short peer_port, std::string self_ip, short self_port);
        int start_listen();
        int stop_listen();
        bool is_connected();
        int connect_peer();
        int disconnect_peer();
        int set_terminate(bool _term);
        int send_data(const void* buffer, int size);
        int recv_data(void* buffer, int size);
};
