#ifndef OKO_OPCODES_H
#define OKO_OPCODES_H

enum Opcodes
{
    OPCODE_NONE = 0x00,

    OP_HELLO = 0x01,
    OP_ACK = 0x02,
    OP_HELLO_RESPONSE = 0x03,
    OP_KEEPALIVE = 0x04,
    OP_LISTROOMS = 0x05,
    OP_JOINROOM = 0x06,
    OP_JOINROOM_RESULT = 0x07,
    OP_CREATEROOM = 0x08,
    OP_CREATEROOM_RESULT = 0x09,
    OP_ROOMSTATUS_REQUEST = 0x0A,
    OP_ROOMSTATUS = 0x0B,               // player list, their cards, game state, score
    OP_PLAYER_JOINED = 0x0C,
    OP_PLAYER_LEFT = 0x0D,
    OP_PLAYER_FORCE_START = 0x0E,
    OP_GAME_BEGINS = 0x0F,
    OP_PLAYER_TURN = 0x10,
    OP_PLAYER_WANNA_CARD = 0x11,
    OP_PLAYER_NO_WANNA_CARD = 0x12,
    OP_PLAYER_GOT_CARD = 0x13,
    OP_UNCOVER_CARD = 0x14,
    OP_GAME_END = 0x15,
    OP_PLAYER_QUIT = 0x16,

    OPCODE_MAX
};

#endif
