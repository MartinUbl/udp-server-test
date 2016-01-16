#ifndef OKO_ROOM_H
#define OKO_ROOM_H

class Client;

#define ROOM_MAX_PLAYERS 4

class Room
{
    public:
        Room(int id);

        bool AddPlayer(Client* cl);
        void RemovePlayer(Client* cl);

        int GetId() { return myId; };

        bool CanBeJoined();

        void BroadcastMessage(int opcode, char* message);

    private:
        Client* clients[ROOM_MAX_PLAYERS];
        int clientCount;
        int myId;
        bool isPlaying;
};

#endif
