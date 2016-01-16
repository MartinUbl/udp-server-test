#include "general.h"
#include "client.h"
#include "roommanager.h"
#include "room.h"

#include <string>

void handleNone(net_message_parsed* msg, Client* cl)
{
    //
}

void handleListRooms(net_message_parsed* msg, Client* cl)
{
    std::list<Room*>* rmlist = sRoomManager->GetRoomList();

    int count = rmlist->size();

    std::string response =  std::to_string(count);

    for (std::list<Room*>::iterator itr = rmlist->begin(); itr != rmlist->end(); ++itr)
    {
        response = response + ";" + std::to_string((*itr)->GetId()) + ";" + ((*itr)->CanBeJoined() ? "1" : "0");
    }

    cl->SendMeDatagram(OP_LISTROOMS, (char*) response.c_str());
}

void handleJoinRoom(net_message_parsed* msg, Client* cl)
{
    //
}

void handleCreateRoom(net_message_parsed* msg, Client* cl)
{
    int res = sRoomManager->CreateRoom(cl);

    if (res < 0)
    {
        cl->SendMeDatagram(OP_CREATEROOM_RESULT, "x");
        return;
    }

    std::string response = std::to_string(res);

    cl->SendMeDatagram(OP_CREATEROOM_RESULT, (char*)response.c_str());
}

void handleRoomStatus(net_message_parsed* msg, Client* cl)
{
    //
}

void handleForceStart(net_message_parsed* msg, Client* cl)
{
    //
}

void handleWannaCard(net_message_parsed* msg, Client* cl)
{
    //
}

void handleDoNotWannaCard(net_message_parsed* msg, Client* cl)
{
    //
}

void handlePlayerQuit(net_message_parsed* msg, Client* cl)
{
    //
}

handler_func_ptr handlers_table[OPCODE_MAX] = {
    handleNone,//OPCODE_NONE
    handleNone,//OP_HELLO
    handleNone,//OP_ACK
    handleNone,//OP_HELLO_RESPONSE
    handleNone,//OP_KEEPALIVE
    handleListRooms,//OP_LISTROOMS
    handleJoinRoom,//OP_JOINROOM
    handleNone,//OP_JOINROOM_RESULT
    handleCreateRoom,//OP_CREATEROOM
    handleNone,//OP_CREATEROOM_RESULT
    handleRoomStatus,//OP_ROOMSTATUS_REQUEST
    handleNone,//OP_ROOMSTATUS
    handleNone,//OP_PLAYER_JOINED
    handleNone,//OP_PLAYER_LEFT
    handleForceStart,//OP_PLAYER_FORCE_START
    handleNone,//OP_GAME_BEGINS
    handleNone,//OP_PLAYER_TURN
    handleWannaCard,//OP_PLAYER_WANNA_CARD
    handleDoNotWannaCard,//OP_PLAYER_NO_WANNA_CARD
    handleNone,//OP_PLAYER_GOT_CARD
    handleNone,//OP_UNCOVER_CARD
    handleNone,//OP_GAME_END
    handlePlayerQuit,//OP_PLAYER_QUIT
};

//
