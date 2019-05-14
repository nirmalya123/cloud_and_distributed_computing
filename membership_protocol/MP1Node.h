/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define MY_DEBUGLOG 0

#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

#define ID_TYPE int // Refer to class MemberListEntry definition
#define PORT_TYPE short
#define HEARTBEAT_TYPE long
#define TIMESTAMP_TYPE long

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    HEARTBEAT,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
    enum MsgTypes msgType;
}MessageHdr;

/**
 * Utility functions
 */
Address get_address_from_id_port(ID_TYPE id, PORT_TYPE port);


/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
    EmulNet *emulNet;
    Log *log;
    Params *par;
    Member *memberNode;
    char NULLADDR[6];

public:
    MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
    Member * getMemberNode() {
        return memberNode;
    }
    int recvLoop();
    static int enqueueWrapper(void *env, char *buff, int size);
    void nodeStart(char *servaddrstr, short serverport);
    int initThisNode(Address *joinaddr);
    int introduceSelfToGroup(Address *joinAddress);
    int finishUpThisNode();
    void nodeLoop();
    void checkMessages();
    bool recvCallBack(void *env, char *data, int size);
    void nodeLoopOps();
    int isNullAddress(Address *addr);
    Address getJoinAddress();
    void initMemberListTable(Member *memberNode);
    void printAddress(Address *addr);
    virtual ~MP1Node();

    bool entry_already_in_membership_list(int id);
    MemberListEntry* get_node_in_mebership_list(int id);

    // JOINREQ message handler
    void send_JOINREQ_msg(Address *introducer_addr);
    void receive_JOINREQ_msg(char *buf);

    // JOINREP message handler
    void send_JOINREP_msg(Address *dest_node_addr);
    void receive_JOINREP_msg(char *buf);

    // HEARTBEAT message handler
    void send_HEARTBEAT_msg(Address *dest_node_addr);
    void receive_HEARTBEAT_msg(char *buf);

    // Functions to handle the membership table to for message communication
    // size_t pack_membership_table(char **buf);
    void pack_membership_table(MessageHdr *msg);
    void unpack_and_process_membership_table(char *);

    // Functions to handle membership table
    void process_membership_table_entry(ID_TYPE id, PORT_TYPE port, HEARTBEAT_TYPE heartbeat, TIMESTAMP_TYPE timestamp);
    void add_entry_to_membership_table(ID_TYPE id, PORT_TYPE port, HEARTBEAT_TYPE heartbeat, TIMESTAMP_TYPE timestamp);
    void remove_entry_from_membership_table(ID_TYPE id);

    // Helper functions for debugging
#if MY_DEBUGLOG == 1
    void print_msg_buffer(char *buf);
    void print_memberList_table();
#endif /* MY_DEBUGLOG */

};

#if MY_DEBUGLOG == 1
#define DPRINT(...) do{                                             \
    printf("\n[");                                                  \
    printAddress(&memberNode->addr);                                \
    printf("][%d][%s:%d]",par->getcurrtime(),__func__,__LINE__);    \
    printf(__VA_ARGS__);                                            \
}while(0)
#else /* MY_DEBUGLOG */
#define DPRINT(...)
#endif /* MY_DEBUGLOG */

#endif /* _MP1NODE_H_ */
