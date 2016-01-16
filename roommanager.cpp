#include "general.h"
#include "client.h"
#include "roommanager.h"
#include "room.h"

RoomManager* RoomManager::m_instance = nullptr;

RoomManager::RoomManager()
{
    maxRoomId = 0;
}

int RoomManager::CreateRoom(Client* first)
{
    if (roomList.size() >= MAX_ROOM_COUNT)
        return -1;

    Room* rm = new Room(++maxRoomId);
    rm->AddPlayer(first);

    roomList.push_back(rm);

    return rm->GetId();
}

int RoomManager::JoinRoom(int id, Client* first)
{
    Room* rm = GetRoom(id);
    if (!rm)
        return -2;

    if (rm->AddPlayer(first))
        return 1;

    return -1;
}

Room* RoomManager::GetRoom(int id)
{
    for (std::list<Room*>::iterator itr = roomList.begin(); itr != roomList.end(); ++itr)
    {
        if ((*itr)->GetId() == id)
            return (*itr);
    }

    return nullptr;
}
