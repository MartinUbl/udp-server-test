#ifndef OKO_CLIENT_H
#define OKO_CLIENT_H

#include "opcodes.h"

// 60 seconds to client kick
#define CLIENT_EXPIRE_INTERVAL 60

struct net_message_parsed;
class Client;

typedef void (*handler_func_ptr)(net_message_parsed*, Client*);

extern handler_func_ptr handlers_table[OPCODE_MAX];

struct client_queue_message
{
    int opcode;
    char* message;
};

class Client
{
    public:
        Client(int id);

        void Start();
        void Run();

        void SignalMonitor();

        void SendMeDatagram(int opcode, char* data);
        void ReceivedDatagram(net_message_parsed* dgram);

        void GetRoom();
        void SetRoomId(int roomId);

    protected:
        int m_id;
        bool m_running;
        time_t m_expireTime;
        std::thread* m_thread;
        std::mutex mtx_client_lock;
        std::condition_variable client_wait_var;

        std::list<client_queue_message*> send_queue;
        std::list<net_message_parsed*> recv_queue;
};

#endif
