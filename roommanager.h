#ifndef OKO_ROOMMANAGER_H
#define OKO_ROOMMANAGER_H

class Client;
class Room;

#define MAX_ROOM_COUNT 50

class RoomManager
{
    public:
        static RoomManager* instance()
        {
            if (m_instance == nullptr)
                m_instance = new RoomManager;

            return m_instance;
        }

        int CreateRoom(Client* first);
        int JoinRoom(int id, Client* first);

        Room* GetRoom(int id);
        std::list<Room*>* GetRoomList() { return &roomList; };

    private:
        RoomManager();
        static RoomManager* m_instance;

        std::list<Room*> roomList;
        int maxRoomId;
};

#define sRoomManager RoomManager::instance()

#endif
