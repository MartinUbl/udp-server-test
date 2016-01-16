#ifndef OKO_NETWORK_H
#define OKO_NETWORK_H

#include "singleton.h"

// how long to wait for ACK to come (before resend)
#define ACK_WAIT_INTERVAL 2000
// how many retries for sending datagram without ACK received
#define ACK_FAIL_RETRY 5

#define OPCODE_LENGTH 3
#define SIZE_LENGTH 2
#define CLIENT_LENGTH 4
#define SEQ_LENGTH 3

struct net_message
{
    char opcode[OPCODE_LENGTH];
    char size[SIZE_LENGTH];
    char client[CLIENT_LENGTH];
    char seq[SEQ_LENGTH];

    char* data;
    int datalen;

    uint16_t checksum;
};

// in case of 0 data, this is the minimal length of message
#define MIN_MESSAGE_LENGTH (OPCODE_LENGTH+SIZE_LENGTH+CLIENT_LENGTH+SEQ_LENGTH+sizeof(uint16_t))

struct net_message_parsed
{
    int opcode;
    int size;
    int client;
    int seq;

    int datasize;
    char* data;
};

class Client;

class Networking
{
    friend class Singleton<Networking>;
    public:
        int Init();

        void ReceiveWorkerFunc();
        void AcceptorWorkerFunc();

        bool SendDatagram(int opcode, int client, char* data, bool waitForAck = true);

        Client* GetClient(int client);
        void RemoveClient(int client);

        net_message_parsed* WaitForIncoming(int client);

    protected:
        Networking();

        void _SendDatagram(char* message, int length);
        int _ParseAndFillReceived(char* buffer, int bufflen);
        net_message* _PrepareDatagramToSend(int opcode, int client, char* data, int* datalen);
        char* _ConvertToStream(net_message* tosend, int datalen);
        void _SendAcknowledge(int client, int seq);

        int _AddClient();

    private:
        fd_set fs;
        SOCKET s;
        sockaddr_in addr;
        std::thread* recv_thread = nullptr;
        std::thread* acceptor_thread = nullptr;

        std::map<int, Client*> client_map;
        int max_client_id;

        std::list<net_message_parsed*> recv_list;
        std::map<int, int> client_ack_list; // highest SEQ acknowledged by client
        std::map<int, int> client_seq_list; // highest SEQ received
        std::mutex net_lock;
        std::condition_variable ack_wait_cond;
        std::condition_variable recv_wait_cond;
        std::condition_variable acceptor_wait_cond;
};

#define sNetwork Singleton<Networking>::instance()

#endif
