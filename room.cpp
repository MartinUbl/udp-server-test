#include "general.h"
#include "room.h"
#include "client.h"

Room::Room(int id)
{
    memset(&clients, 0, sizeof(Client*) * ROOM_MAX_PLAYERS);
    myId = id;
    isPlaying = false;
}

bool Room::AddPlayer(Client* cl)
{
    if (clientCount >= ROOM_MAX_PLAYERS)
        return false;

    for (int i = 0; i < ROOM_MAX_PLAYERS; i++)
    {
        if (clients[i] == nullptr)
        {
            clients[i] = cl;
            clientCount++;
            break;
        }
    }

    return true;
}

void Room::RemovePlayer(Client* cl)
{
    for (int i = 0; i < ROOM_MAX_PLAYERS; i++)
    {
        if (clients[i] == cl)
        {
            clients[i] = nullptr;
            clientCount--;
            break;
        }
    }
}

void Room::BroadcastMessage(int opcode, char* message)
{
    for (int i = 0; i < ROOM_MAX_PLAYERS; i++)
    {
        if (clients[i] == nullptr)
            continue;

        clients[i]->SendMeDatagram(opcode, message);
    }
}

bool Room::CanBeJoined()
{
    return (!isPlaying && clientCount < ROOM_MAX_PLAYERS);
}
