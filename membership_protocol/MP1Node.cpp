/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 *              Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

Address get_address_from_id_port(ID_TYPE id, PORT_TYPE port) {
    return Address(to_string(id) + ":" + to_string(port));
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
    for( int i = 0; i < 6; i++ ) {
        NULLADDR[i] = 0;
    }
    this->memberNode = member;
    this->emulNet = emul;
    this->log = log;
    this->par = params;
    this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 *              This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
        return false;
    }
    else {
        return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
    Queue q;
    return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 *              All initializations routines for a member.
 *              Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }
    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
    /*
     * This function is partially implemented and may require changes
     */
    int id = *(int*)(&memberNode->addr.addr);
    int port = *(short*)(&memberNode->addr.addr[4]);

    memberNode->bFailed = false;
    memberNode->inited = true;
    memberNode->inGroup = false;
    // node is up!
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->pingCounter = TFAIL;
    memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    // Add self to own's membership table
    add_entry_to_membership_table(id, port, memberNode->heartbeat, par->getcurrtime());

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
    MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    // TODO - Try to introduce to more than one introducer
    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
        DPRINT("Starting up group as [");
        printAddress(joinaddr);
        DPRINT("]");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
        DPRINT("[%p] msg",msg);
        DPRINT("[%p] (char *)(msg+1)",(char *)(msg+1));
        DPRINT("[%p] (char *)(msg+1) + 1 + sizeof(memberNode->addr.addr)",(char *)(msg+1) + 1 + sizeof(memberNode->addr.addr));
        DPRINT("");
#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif
        // TODO - Try to introduce to more than one introducer
        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    memberNode->inited = false;
    memberNode->inGroup = false;
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->pingCounter = TFAIL;
    memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 *              Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
        return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
        return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
        ptr = memberNode->mp1q.front().elt;
        size = memberNode->mp1q.front().size;
        memberNode->mp1q.pop();
        recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
    /*
     * Your code goes here
     */
    MessageHdr* received_msg = (MessageHdr*) malloc(size * sizeof(char));
    memcpy(received_msg, data, sizeof(MessageHdr));
    // // DPRINT("---");
    switch(received_msg->msgType) {
        case JOINREQ:
            /**
             * Only the introducer node executes this block
             * Retrieve id, port, heartbeat from the msg
             * If entry does not exists then create and add new entry
             * Send JOINREP msg
             */
            receive_JOINREQ_msg(data);
            // receive_JOINREQ_msg(data + sizeof(MessageHdr));
            break;
        case JOINREP:
            /**
             * All the nodes (may be execluding the introducer receives this msg)
             * Set inGroup to True
             * Unpack and process the memberList received
             */
            memberNode->inGroup = true;
            receive_JOINREP_msg(data);
            // receive_JOINREP_msg(data + sizeof(MessageHdr));
            break;
        case HEARTBEAT:
            /**
             * All nodes receives this msg
             * Unpack and process the memberList received
             */
            receive_HEARTBEAT_msg(data);
            // receive_HEARTBEAT_msg(data + sizeof(MessageHdr));
            break;
    }
    return true; // Return value is not used
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 *              the nodes
 *              Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

    /*
     * Your code goes here
     */

    /**
     * Process the memberList to remove the failed node after TREMOVE
     * Update own entry's heartbeat
     * Send heartbeat msg
     * Update ping counter and timeout counter
     */

    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
        Address node_address = get_address_from_id_port(i->id, i->getport());
        if(!(memberNode->addr == node_address)) {
            /**
             * If the difference between par->getcurrtime() and node's timestamp
             * is greater than TREMOVE then remove node from membership list
             */
            DPRINT("par->getcurrtime[%ld] i->gettimestamp[%ld] TREMOVE [%ld]",par->getcurrtime(), i->gettimestamp(),TREMOVE);
            if(par->getcurrtime() - i->gettimestamp() > TREMOVE) { // TODO Try >= here
                DPRINT("REMOVED [%d]",i->id);
                // Remove node from membership list
                memberNode->memberList.erase(i);
#ifdef DEBUGLOG
                log->logNodeRemove(&memberNode->addr, &node_address);
#endif
            }
        }
    }
    // Increment overall counter
    // memberNode->timeOutCounter++;

    // Check if node should send a heartbeat msg
    if(memberNode->pingCounter == 0)
    {
        // Increment heartbeat
        int id = *(int*)(memberNode->addr.addr);
        memberNode->heartbeat++;
        MemberListEntry* entry = get_node_in_mebership_list(id);
        entry->setheartbeat(memberNode->heartbeat);

        // Send heartbeat messages to all nodes // TODO : try selecting random number of nnb entries
        for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
            Address nodeAddress = get_address_from_id_port(i->id, i->getport());
            if(!(memberNode->addr == nodeAddress)) {
                send_HEARTBEAT_msg(&nodeAddress);
            }
        }
        // Reset ping counter
        memberNode->pingCounter = TFAIL;
    }
    else {
        // Decrement ping counter
        memberNode->pingCounter--;
    }
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
    return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
    memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;
    // printf("[%hd%hd%hd%hd%hd%hd]",addr->addr[0],addr->addr[1],addr->addr[2],addr->addr[3],addr->addr[4],addr->addr[5]);
}

bool MP1Node::entry_already_in_membership_list(int id) {
    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
        if(i->id == id) {
            return true;
        }
    }
    return false;
}

MemberListEntry* MP1Node::get_node_in_mebership_list(int id) {
    MemberListEntry* entry = NULL;

    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
        if(i->id == id) {
            entry = i.base();
            break;
        }
    }
    return entry;
}

// JOINREQ message handler
void MP1Node::send_JOINREQ_msg(Address *introducer_addr) {
    // TODO
    return;
}

void MP1Node::receive_JOINREQ_msg(char *buf) {
    ID_TYPE id = 0;
    PORT_TYPE port = 0;
    HEARTBEAT_TYPE heartbeat = 0;
    int offset = 0;
    Address recv_addr;
    memcpy(&recv_addr, buf + sizeof(MessageHdr), sizeof(recv_addr));
    offset += sizeof(recv_addr);

    memcpy(&heartbeat, buf + 1 + sizeof(MessageHdr) + offset, sizeof(HEARTBEAT_TYPE));
    // printAddress(&recv_addr);
    id = *(int*)(&recv_addr.addr);
    port = *(short*)(&recv_addr.addr[4]);
    DPRINT("[%d]-[%hu]-[%ld]\n",id,port,heartbeat);
    DPRINT("[%p] buf", buf);
    DPRINT("[%p] buf + sizeof(MessageHdr)", buf + sizeof(MessageHdr));
    DPRINT("[%p] buf + 1 + sizeof(MessageHdr) + offset", buf + 1 + sizeof(MessageHdr) + offset);

    // Create new membership entry and add to the membership list of the node
    TIMESTAMP_TYPE time_stamp = par->getcurrtime();
    if (entry_already_in_membership_list(id)) {
        process_membership_table_entry(id, port, heartbeat, time_stamp);
    } else {
        add_entry_to_membership_table(id, port, heartbeat, time_stamp);
    }

    // Send JOINREP message
    Address dest_addr = get_address_from_id_port(id, port);
#if MY_DEBUGLOG == 1
    print_memberList_table();
#endif /* MY_DEBUGLOG */
    send_JOINREP_msg(&dest_addr);

}

// JOINREP message handler
void MP1Node::send_JOINREP_msg(Address *dest_node_addr) {
    size_t memberList_entry_size = sizeof(ID_TYPE) + sizeof(PORT_TYPE) + sizeof(HEARTBEAT_TYPE) + sizeof(TIMESTAMP_TYPE);
    size_t msg_size = sizeof(MessageHdr) + sizeof(memberNode->memberList.size()) + (memberNode->memberList.size() * memberList_entry_size);
    MessageHdr* msg = (MessageHdr*) malloc(msg_size * sizeof(char));
    msg->msgType = JOINREP;
#if MY_DEBUGLOG == 1
    print_memberList_table();
#endif /* MY_DEBUGLOG */

    // Serialize member list
    pack_membership_table(msg);

    emulNet->ENsend(&memberNode->addr, dest_node_addr, (char*)msg, msg_size);

    if (msg)
        free(msg);
}

void MP1Node::receive_JOINREP_msg(char *buf) {
    unpack_and_process_membership_table(buf);
#if MY_DEBUGLOG == 1
    print_memberList_table();
#endif /* MY_DEBUGLOG */
}

// HEARTBEAT message handler
void MP1Node::send_HEARTBEAT_msg(Address *dest_node_addr) {
    size_t memberList_entry_size = sizeof(ID_TYPE) + sizeof(PORT_TYPE) + sizeof(HEARTBEAT_TYPE) + sizeof(TIMESTAMP_TYPE);
    size_t msg_size = sizeof(MessageHdr) + sizeof(memberNode->memberList.size()) + (memberNode->memberList.size() * memberList_entry_size);
    MessageHdr* msg = (MessageHdr*) malloc(msg_size * sizeof(char));
    msg->msgType = HEARTBEAT;
#if MY_DEBUGLOG == 1
    print_memberList_table();
#endif /* MY_DEBUGLOG */

    // Serialize member list
    pack_membership_table(msg);

    emulNet->ENsend(&memberNode->addr, dest_node_addr, (char*)msg, msg_size);

    if (msg)
        free(msg);
}
void MP1Node::receive_HEARTBEAT_msg(char *buf) {
    unpack_and_process_membership_table(buf);
#if MY_DEBUGLOG == 1
    print_memberList_table();
#endif /* MY_DEBUGLOG */
}

void MP1Node::pack_membership_table(MessageHdr *msg) {
    /**
     * Format after serialization
     * - Number of entries
     *      -[id, port, heartbeat, timestamp]
     *      -[id, port, heartbeat, timestamp]
     *      -[id, port, heartbeat, timestamp]
     *      - ...
     */
    vector<MemberListEntry> *memberList = &this->memberNode->memberList;
    int num_row_entries = memberList->size();
    size_t size_of_row = sizeof(ID_TYPE) + sizeof(PORT_TYPE) + sizeof(HEARTBEAT_TYPE) + sizeof(TIMESTAMP_TYPE);
    size_t buf_size = sizeof(num_row_entries) + (num_row_entries * size_of_row);
    int msg_type1 = 1;
    memset((char *)(msg + msg_type1), 0, buf_size);
    memcpy((char *)(msg + msg_type1), &num_row_entries, sizeof(int));
    DPRINT("[%p] msg", msg);
    DPRINT("[%p] (char *)(msg + msg_type1) num_row_entries", (char *)(msg + msg_type1));
    // Serialize member list entries
    int offset = sizeof(num_row_entries);
    DPRINT("num_row_entries[%d]",num_row_entries);

    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
        DPRINT("Adding [%d]-[%hd]-[%ld]-[%ld] offset[%d]", i->id, i->port, i->heartbeat, i->timestamp,offset);
        memcpy((char *)(msg + msg_type1) + offset, &i->id, sizeof(ID_TYPE));
        DPRINT("[%p] (char *)(msg + msg_type1) + offset id", (char *)(msg + msg_type1) + offset);
        offset += sizeof(ID_TYPE);

        memcpy((char *)(msg + msg_type1) + offset, &i->port, sizeof(PORT_TYPE));
        DPRINT("[%p] (char *)(msg + msg_type1) + offset port", (char *)(msg + msg_type1) + offset);
        offset += sizeof(PORT_TYPE);

        memcpy((char *)(msg + msg_type1) + offset, &i->heartbeat, sizeof(HEARTBEAT_TYPE));
        DPRINT("[%p] (char *)(msg + msg_type1) + offset heartbeat", (char *)(msg + msg_type1) + offset);
        offset += sizeof(HEARTBEAT_TYPE);

        memcpy((char *)(msg + msg_type1) + offset, &i->timestamp, sizeof(TIMESTAMP_TYPE));
        DPRINT("[%p] (char *)(msg + msg_type1) + offset timestamp", (char *)(msg + msg_type1) + offset);
        offset += sizeof(TIMESTAMP_TYPE);
    }
#if MY_DEBUGLOG == 1
    print_msg_buffer((char *)(msg + msg_type1));
#endif /* MY_DEBUGLOG */

}

void MP1Node::unpack_and_process_membership_table(char *buf){
    int num_of_row = 0;
    ID_TYPE id;
    PORT_TYPE port;
    HEARTBEAT_TYPE heartbeat;
    TIMESTAMP_TYPE timestamp;

    memcpy(&num_of_row, buf + sizeof(MessageHdr), sizeof(num_of_row));

    // Deserialize member list entries
    int offset = sizeof(num_of_row);

    for(int i = 0; i < num_of_row; i++) {
        id = 0;
        port  = 0;
        heartbeat = 0;
        timestamp = 0;

        memcpy(&id, buf + sizeof(MessageHdr) + offset, sizeof(ID_TYPE));
        offset += sizeof(ID_TYPE);

        memcpy(&port, buf + sizeof(MessageHdr) + offset, sizeof(PORT_TYPE));
        offset += sizeof(PORT_TYPE);

        memcpy(&heartbeat, buf + sizeof(MessageHdr) + offset, sizeof(HEARTBEAT_TYPE));
        offset += sizeof(HEARTBEAT_TYPE);

        memcpy(&timestamp, buf + sizeof(MessageHdr) + offset, sizeof(TIMESTAMP_TYPE));
        offset += sizeof(TIMESTAMP_TYPE);

        if (entry_already_in_membership_list(id)) {
            TIMESTAMP_TYPE time_stamp = par->getcurrtime();
            process_membership_table_entry(id, port, heartbeat, time_stamp);
        } else {
            add_entry_to_membership_table(id, port, heartbeat, timestamp);
        }
    }
}

void MP1Node::process_membership_table_entry(ID_TYPE id, PORT_TYPE port, HEARTBEAT_TYPE heartbeat, TIMESTAMP_TYPE timestamp) {
    /**
     * If Entry already exists then compare heartbeat and time stamp - update/ delete node as an when needed
     * If entry does not exists then add the new entry
     */
    MemberListEntry* entry = get_node_in_mebership_list(id);
    DPRINT("id[%d] par->getcurrtime[%ld] entry->gettimestamp[%ld] timestamp[%ld]",id, par->getcurrtime(), entry->gettimestamp(), timestamp);
    if(par->getcurrtime() - entry->gettimestamp() < TFAIL or par->getcurrtime() - timestamp < TFAIL) {
        if (entry->getheartbeat() < heartbeat) {
            DPRINT("Node [%d] HEARTBEAT updated to [%ld]",id, heartbeat);
            entry->setheartbeat(heartbeat);
            entry->settimestamp(timestamp);
        }
    }
}

void MP1Node::add_entry_to_membership_table(ID_TYPE id, PORT_TYPE port, HEARTBEAT_TYPE heartbeat, TIMESTAMP_TYPE timestamp) {
    Address new_node_addr = get_address_from_id_port(id, port);

    // If new node is not in the member list table then create and add a new member list entry
    MemberListEntry* newEntry = new MemberListEntry(id, port, heartbeat, timestamp);

    memberNode->memberList.insert(memberNode->memberList.end(), *newEntry);
#ifdef DEBUGLOG
    log->logNodeAdd(&memberNode->addr, &new_node_addr);
#endif

    delete newEntry;
}

void MP1Node::remove_entry_from_membership_table(ID_TYPE id) {
    MemberListEntry* entry = get_node_in_mebership_list(id);
    PORT_TYPE port = entry->getport();
    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {
        if(i->id == id) {
            memberNode->memberList.erase(i);
#ifdef DEBUGLOG
            Address nodeToRemoveAddress = get_address_from_id_port(id, port);
            log->logNodeRemove(&memberNode->addr, &nodeToRemoveAddress);
#endif
            break;
        }
    }
}

#if MY_DEBUGLOG == 1
/**
 * Heartbeat message format
 * - MSG Type
 * - Number of entries
 *      -[id, port, heartbeat, timestamp]
 *      -[id, port, heartbeat, timestamp]
 *      -[id, port, heartbeat, timestamp]
 *      - ...
 */
void MP1Node::print_msg_buffer(char *buf) {
    int num_of_row = 0;
    ID_TYPE id = 0;
    PORT_TYPE port;
    HEARTBEAT_TYPE heartbeat;
    TIMESTAMP_TYPE timestamp;
    DPRINT("--------------------------------------------------------");
    memcpy(&num_of_row, buf , sizeof(num_of_row));
    DPRINT("|    Number of rows [%d]",num_of_row);

    int offset = sizeof(num_of_row);

    for(int i = 0; i < num_of_row; i++) {
        id = 0;
        port  = 0;
        heartbeat = 0;
        timestamp = 0;

        memcpy(&id, buf  + offset, sizeof(ID_TYPE));
        offset += sizeof(ID_TYPE);

        memcpy(&port, buf  + offset, sizeof(PORT_TYPE));
        offset += sizeof(PORT_TYPE);

        memcpy(&heartbeat, buf  + offset, sizeof(HEARTBEAT_TYPE));
        offset += sizeof(HEARTBEAT_TYPE);

        memcpy(&timestamp, buf  + offset, sizeof(TIMESTAMP_TYPE));
        offset += sizeof(TIMESTAMP_TYPE);

        DPRINT("|    [%d] - [%d] - [%hd] - [%ld] - [%ld]",i, id, port, heartbeat, timestamp);
    }

    DPRINT("--------------------------------------------------------");

}

void MP1Node::print_memberList_table(){
    DPRINT("--------------------------------------------------------\n");
    printAddress(&memberNode->addr);
    for(std::vector<MemberListEntry>::iterator i = memberNode->memberList.begin(); i != memberNode->memberList.end(); ++i) {

        DPRINT("|    [%d]-[%hd]-[%ld]-[%ld]", i->id, i->port, i->heartbeat, i->timestamp);
    }
    DPRINT("--------------------------------------------------------");
}
#endif /* MY_DEBUGLOG */
