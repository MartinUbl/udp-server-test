#include "general.h"
#include "client.h"
#include "network.h"
#include "opcodes.h"

Client::Client(int id) : m_id(id), m_running(true), m_expireTime(0)
{
    //
}

void Client::Start()
{
    m_thread = new std::thread(&Client::Run, this);
}

void Client::Run()
{
    net_message_parsed* msg_in;
    client_queue_message* msg_out;
    bool active;
    int waittime;

    std::list<net_message_parsed*> tohandle;

    while (!m_running)
        std::this_thread::yield();

    waittime = 5;

    while (m_running && (!m_expireTime || m_expireTime > time(nullptr)))
    {
        // client lock scope
        {
            std::unique_lock<std::mutex> lck(mtx_client_lock);

            // wait for signal from monitor
            client_wait_var.wait_for(lck, std::chrono::seconds(waittime));

            waittime = (m_expireTime) ? 15 : 5;

            active = false;

            // check recv queue, handle packets
            for (std::list<net_message_parsed*>::iterator itr = recv_queue.begin(); itr != recv_queue.end();)
            {
                msg_in = *itr;

                if (msg_in->opcode > OPCODE_NONE && msg_in->opcode < OPCODE_MAX)
                {
                    std::cout << "Handling of packet opcode " << msg_in->opcode << std::endl;
                    tohandle.push_back(msg_in);
                }
                else
                {
                    std::cout << "Invalid opcode received" << std::endl;
                    // TODO: kick player
                    delete msg_in;
                }

                active = true;

                itr = recv_queue.erase(itr);
            }

            // check send queue, handle sending
            for (std::list<client_queue_message*>::iterator itr = send_queue.begin(); itr != send_queue.end();)
            {
                msg_out = *itr;

                if (!sNetwork->SendDatagram(msg_out->opcode, m_id, msg_out->message))
                {
                    m_expireTime = time(nullptr) + CLIENT_EXPIRE_INTERVAL;

                    // TODO: broadcast to room about inactivity
                }
                else
                    m_expireTime = 0;

                delete msg_out;

                active = true;

                itr = send_queue.erase(itr);
            }
        }

        if (tohandle.size() > 0)
        {
            for (std::list<net_message_parsed*>::iterator itr = tohandle.begin(); itr != tohandle.end(); ++itr)
            {
                msg_in = *itr;
                handlers_table[msg_in->opcode](msg_in, this);
                delete msg_in;
            }

            tohandle.clear();
        }

        // if not active, send keepalive next turn
        if (!active && !m_expireTime)
        {
            // queue keepalive datagram
            SendMeDatagram(OP_KEEPALIVE, "");
            // and schedule next update after 1s
            waittime = 1;
        }
    }

    if (m_expireTime)
        std::cout << "Client session " << m_id << " expired, disconnecting" << std::endl;
    else
        std::cout << "Client " << m_id << " disconnected" << std::endl;

    // TODO: broadcast to room about leave

    sNetwork->RemoveClient(m_id);
}

void Client::SignalMonitor()
{
    std::unique_lock<std::mutex> lck(mtx_client_lock);

    client_wait_var.notify_all();
}

void Client::SendMeDatagram(int opcode, char* data)
{
    client_queue_message* msg = new client_queue_message;

    msg->opcode = opcode;

    int lms = strlen(data);

    msg->message = new char[lms + 1];
    memcpy(msg->message, data, lms);
    msg->message[lms] = '\0';

    // client lock scope
    {
        std::unique_lock<std::mutex> lck(mtx_client_lock);

        send_queue.push_back(msg);
    }

    SignalMonitor();
}

void Client::ReceivedDatagram(net_message_parsed* dgram)
{
    // client lock scope
    {
        std::unique_lock<std::mutex> lck(mtx_client_lock);

        recv_queue.push_back(dgram);
    }

    SignalMonitor();
}
