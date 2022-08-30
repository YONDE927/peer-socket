//仕様
//peerはipとportのセット
//クライアントとサーバーを意識しないバックエンド(設定除く)
//接続が切れたことを検知すれば、再接続スレッドを立ち上げる
//接続が切れたかどうかはsend,recvで検知する
//ioはオフライン時に再接続までブロックするか選択できる
//socket_fdは常に排他制御する
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
//joinでスレッド回収するとjoin側がブロックされてしまうので
//detachで管理は手放し、メンバ変数でスレッドの空き状態を管理する
//
//終了手順
//out of bandによるシグナルで終了することも考えたが、面倒なので
//ユーザー側に責任転嫁する。

#pragma once
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#define TRY_INTERVAL 1

struct peer_info{
    std::string ip;
    short port{0};
    sockaddr_in addr;
    int listen_fd{0};
};

class peer_socket{
    private:
        enum mode_t{
            UNSET,
            CLIENT,
            SERVER
        }mode{UNSET};
        enum status_t{
            DISCONNECTED,
            CONNECTED,
            CONNECTING
        }status{DISCONNECTED};
        bool term{false};
        int fd{0};
        peer_info peer;
        peer_info self;
        std::mutex mtx;
        std::condition_variable cond;
    public:
        peer_socket();
        ~peer_socket();
        void setup(mode_t m, std::string peer_ip, short peer_port, std::string self_ip, short self_port);
        void start_connecting();
        int close_peer();
        int close_self();
        int start_listen();
        int stop_listen();
        bool is_connected();
        void wait_connect();
        int send_data(const void* buffer, int size);
        int recv_data(void* buffer, int size);
    private:
        void try_connect_entry();
        int try_connect_client();
        int try_connect_server();
        int try_connect();
};
