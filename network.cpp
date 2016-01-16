#include "general.h"
#include "network.h"
#include "helpers.h"
#include "opcodes.h"
#include "client.h"

Networking::Networking()
{
    max_client_id = 0;
}

int Networking::Init()
{
    int res;

    WORD version = MAKEWORD(1, 1);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
    {
        std::cout << "Failed to start winsocks" << std::endl;
        return -1;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        std::cout << "Error creating socket" << std::endl;
        return -1;
    }

    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);

    res = bind(s, (sockaddr*)&addr, sizeof(addr));
    if (res < 0)
    {
        std::cout << "Failed to bind" << std::endl;
        return -1;
    }

    FD_ZERO(&fs);
    FD_SET(s, &fs);

    recv_thread = new std::thread(&Networking::ReceiveWorkerFunc, this);
    acceptor_thread = new std::thread(&Networking::AcceptorWorkerFunc, this);

    return 0;
}

int Networking::_ParseAndFillReceived(char* buffer, int buff_size)
{
    int a;

    if (buff_size >= MIN_MESSAGE_LENGTH)
    {
        char* opcode = buffer;
        char* size = opcode + OPCODE_LENGTH;
        char* client = size + SIZE_LENGTH;
        char* seq = client + CLIENT_LENGTH;
        char* datasec = seq + SEQ_LENGTH;

        int op = convert_pchar_to_uint(opcode, OPCODE_LENGTH);
        int sz = convert_pchar_to_uint(size, SIZE_LENGTH);
        int cl = convert_pchar_to_uint(client, CLIENT_LENGTH);
        int sq = convert_pchar_to_uint(seq, SEQ_LENGTH);

        // if some non-numeric character supplied, it is an error
        if (op == -1 || sz == -1 || cl == -1 || sq == -1)
        {
            std::cout << "Non-numeric character in opcode, size, sequence or client, discarding" << std::endl;
            return -1;
        }

        if (buff_size < sz)
        {
            std::cout << "Not enough data supplied, discarding" << std::endl;
            return -1;
        }

        net_message_parsed* ps = new net_message_parsed;
        ps->opcode = op;
        ps->size = sz;
        ps->client = cl;
        ps->seq = sq;

        int datalen = sz - MIN_MESSAGE_LENGTH + 1;

        ps->data = new char[datalen];
        memset(ps->data, 0, datalen);
        memcpy(ps->data, datasec, datalen - 1);
        ps->data[datalen - 1] = '\0';
        ps->datasize = datalen;

        uint16_t chsum = 0;
        for (a = 0; a < (int)(sz - sizeof(uint16_t)); a++)
            chsum += (uint16_t)buffer[a];

        if ((chsum & 0x00FF) == 0x00)
            chsum |= 0x0001;
        if ((chsum & 0xFF00) == 0x00)
            chsum |= 0x0100;

        uint16_t pkt_chsum = *((uint16_t*)(buffer + sz - sizeof(uint16_t)));

        if (chsum != pkt_chsum)
        {
            std::cout << "Invalid checksum " << pkt_chsum << ", expected " << chsum << std::endl;
            return -1;
        }

        {
            if (ps->opcode == OP_ACK)
            {
                int seqack = atoi(ps->data);

                std::cout << "Received ACK; seq: " << seqack << std::endl;

                std::unique_lock<std::mutex> lck(net_lock);

                if (client_ack_list.find(ps->client) == client_ack_list.end())
                    client_ack_list[ps->client] = 1; // first expected ACK is 1

                if (client_ack_list[ps->client] == seqack - 1)
                    client_ack_list[ps->client] = seqack;

                // wake up all clients waiting for acknowledge
                ack_wait_cond.notify_all();
            }
            else
            {
                net_lock.lock();

                std::cout << "Received datagram; opcode " << ps->opcode << ", client: " << ps->client << ", datalen: " << ps->datasize << ", data: " << ps->data << std::endl;

                if (client_seq_list.find(ps->client) == client_seq_list.end())
                    client_seq_list[ps->client] = ps->seq;

                if (client_seq_list[ps->client] == ps->seq || ps->opcode == OP_HELLO)
                {
                    std::cout << "Received datagram; opcode " << ps->opcode << ", data: " << ps->data << ", handling" << std::endl;

                    if (ps->opcode != OP_HELLO)
                        client_seq_list[ps->client] += 1;

                    Client* cl;

                    if (ps->client > 0 && (cl = GetClient(ps->client)) != nullptr)
                        cl->ReceivedDatagram(ps);
                    else
                    {
                        recv_list.push_back(ps);
                        recv_wait_cond.notify_all();
                    }

                    net_lock.unlock();

                    _SendAcknowledge(ps->client, ps->seq);
                }
                else if (client_seq_list[ps->client] > ps->seq)
                {
                    std::cout << "Received datagram; opcode " << ps->opcode << ", data: " << ps->data << ", sending only ACK" << std::endl;

                    net_lock.unlock();

                    _SendAcknowledge(ps->client, ps->seq);
                }
                else
                    net_lock.unlock();
            }
        }

        return 0;
    }

    std::cout << "Invalid datagram length, discarding" << std::endl;

    // -1 = not parsed
    return -1;
}

net_message* Networking::_PrepareDatagramToSend(int opcode, int client, char* data, int* datalen)
{
    int a;

    net_message* tosend = new net_message;

    convert_uint_to_pchar(opcode, tosend->opcode, OPCODE_LENGTH);
    convert_uint_to_pchar(MIN_MESSAGE_LENGTH + strlen(data), tosend->size, SIZE_LENGTH);
    convert_uint_to_pchar(client, tosend->client, CLIENT_LENGTH);
    tosend->data = data;

    *datalen = strlen(data);

    uint16_t chsum = 0;
    for (a = 0; a < OPCODE_LENGTH; a++)
        chsum += tosend->opcode[a];
    for (a = 0; a < SIZE_LENGTH; a++)
        chsum += tosend->size[a];
    for (a = 0; a < CLIENT_LENGTH; a++)
        chsum += tosend->client[a];
    for (a = 0; a < SEQ_LENGTH; a++)
        chsum += tosend->seq[a];
    for (a = 0; a < (int)strlen(data); a++)
        chsum += tosend->data[a];

    if ((chsum & 0x00FF) == 0x0000)
        chsum |= 0x0001;
    if ((chsum & 0xFF00) == 0x0000)
        chsum |= 0x0100;

    tosend->checksum = chsum;

    return tosend;
}

char* Networking::_ConvertToStream(net_message* tosend, int datalen)
{
    char* dst = new char[MIN_MESSAGE_LENGTH + datalen];

    memcpy(dst, tosend->opcode, OPCODE_LENGTH);
    memcpy(dst + OPCODE_LENGTH, tosend->size, SIZE_LENGTH);
    memcpy(dst + OPCODE_LENGTH + SIZE_LENGTH, tosend->client, CLIENT_LENGTH);
    memcpy(dst + OPCODE_LENGTH + SIZE_LENGTH + CLIENT_LENGTH, tosend->seq, SEQ_LENGTH);
    memcpy(dst + OPCODE_LENGTH + SIZE_LENGTH + CLIENT_LENGTH + SEQ_LENGTH, tosend->data, datalen);
    memcpy(dst + OPCODE_LENGTH + SIZE_LENGTH + CLIENT_LENGTH + SEQ_LENGTH + datalen, &tosend->checksum, sizeof(uint16_t));

    return dst;
}

void Networking::ReceiveWorkerFunc()
{
    fd_set rcfs;

    timeval tc;
    tc.tv_sec = 180;
    tc.tv_usec = 0;

    char rdbuf[1024];
    int len = sizeof(addr);

    while (true)
    {
        memcpy(&rcfs, &fs, sizeof(fs));

        int a = select(s, &rcfs, nullptr, nullptr, &tc);

        if (a == 0)
            continue;

        if (FD_ISSET(s, &rcfs))
        {
            memset(&rdbuf, 0, 1024);
            recvfrom(s, rdbuf, 1024, 0, (sockaddr*)&addr, &len);

            _ParseAndFillReceived(rdbuf, 1024);
        }
    }
}

void Networking::AcceptorWorkerFunc()
{
    net_message_parsed* incoming;
    int clientId;
    Client* cl;
    char respId[CLIENT_LENGTH+1];

    while (true)
    {
        incoming = WaitForIncoming(0);

        if (!incoming)
            continue;

        if (incoming->opcode != OP_HELLO)
            continue;

        clientId = _AddClient();

        cl = GetClient(clientId);
        if (!cl)
            continue;

        cl->Start();

        convert_uint_to_pchar(clientId, respId, CLIENT_LENGTH);
        respId[CLIENT_LENGTH] = '\0';

        cl->SendMeDatagram(OP_HELLO_RESPONSE, respId);
    }
}

void Networking::_SendDatagram(char* message, int length)
{
    fd_set wfs;

    timeval tc;
    tc.tv_sec = 2;
    tc.tv_usec = 3000;

    int len = sizeof(addr);

    memcpy(&wfs, &fs, sizeof(fs));

    int a = select(s, nullptr, &wfs, nullptr, &tc);

    if (a > 0 && FD_ISSET(s, &wfs))
    {
        sendto(s, message, length, 0, (sockaddr*)&addr, len);
    }
}

bool Networking::SendDatagram(int opcode, int client, char* data, bool waitForAck)
{
    int ll;
    int seq;

    if (client_ack_list.find(client) == client_ack_list.end())
        client_ack_list[client] = 0;

    client_ack_list[client] += 1;

    seq = client_ack_list[client];

    net_message* msg = _PrepareDatagramToSend(opcode, client, data, &ll);
    convert_uint_to_pchar(seq, msg->seq, SEQ_LENGTH);
    char* buf = _ConvertToStream(msg, ll);
    delete msg;

    ll += MIN_MESSAGE_LENGTH;

    // some retries
    for (int i = 0; i < ACK_FAIL_RETRY; i++)
    {
        std::unique_lock<std::mutex> lck(net_lock);

        _SendDatagram(buf, ll);

        // typically when sending ACK, do not wait for ACK ACK
        if (!waitForAck)
            return true;

        uint32_t beginTime = getMSTime();

        while (ack_wait_cond.wait_for(lck, std::chrono::milliseconds(ACK_WAIT_INTERVAL - getMSTimeDiff(beginTime, getMSTime()))) == std::cv_status::no_timeout)
        {
            // look to ack queue
            // if present, return

            // we received what we need
            if (client_ack_list.find(client) != client_ack_list.end() && client_ack_list[client] == seq)
            {
                std::cout << "ACK received" << std::endl;
                return true;
            }
        }

        std::cout << "ACK (" << seq <<") not received, retrying..." << (i+1) << std::endl;
    }

    return false;
}

void Networking::_SendAcknowledge(int client, int seq)
{
    char res[6];
    memset(res, 0, 6);
    _itoa(seq, res, 10);

    SendDatagram(OP_ACK, client, res, false);
}

net_message_parsed* Networking::WaitForIncoming(int client)
{
    while (true)
    {
        std::unique_lock<std::mutex> lck(net_lock);

        for (std::list<net_message_parsed*>::iterator itr = recv_list.begin(); itr != recv_list.end(); ++itr)
        {
            if ((*itr)->client == client)
            {
                net_message_parsed* pp = *itr;
                recv_list.erase(itr);
                return pp;
            }
        }

        recv_wait_cond.wait(lck);
    }
}

Client* Networking::GetClient(int client)
{
    if (client_map.find(client) == client_map.end())
        return nullptr;

    return client_map[client];
}

void Networking::RemoveClient(int client)
{
    if (client_map.find(client) != client_map.end())
        client_map.erase(client);

    if (client_ack_list.find(client) != client_ack_list.end())
        client_ack_list.erase(client);

    if (client_seq_list.find(client) != client_seq_list.end())
        client_seq_list.erase(client);
}

int Networking::_AddClient()
{
    int clientId = ++max_client_id;

    client_map[clientId] = new Client(clientId);

    return clientId;
}
