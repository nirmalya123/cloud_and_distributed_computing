/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"

#include <list>
#include <utility>
#include <vector>
#include <algorithm>

#define NUM_OF_REPLICAS 3  // enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};
#define QUORUM_COUNT ((NUM_OF_REPLICAS)/2+1)
#define INVALID_KEY "invalidKey"
#define NULL_VALUE ""

#define RESPONSE_WAIT_TIME 20  // Refer to #define TREMOVE 20 in MP1Node.h

typedef struct transaction_entry {
    int time_stamp;
    int quorum_count;
    MessageType message_type;
    string key;
    string value;
}TransactionEntry;

typedef struct read_value_entry {
    int time_stamp = 0;
    string value;
    bool read_repair_needed = false;
}ReadValueEntry;
/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 *
 *      Each node hold replicas of previous two nodes.
 */
class MP2Node {
private:
    bool init11;
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
    // int my_loc_in_ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
    // Handle Transactions
    // list<Message> transaction_list; // TODO : try map <transaction_id+msg type, Messege>
    // map<int, pair<int, MessageType>> transaction_quorum;  // transaction_id, <quorum_count, MessageType>
    map<int, TransactionEntry> transaction_quorum;  // transaction_id, <quorum_count, MessageType>
    map<int, ReadValueEntry> value_ts_map;  // transaction_id, <timestamp, value>
public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

    // Messege send utilities
    // void send_messege(Message message,vector<Node>& recipients);
    void send_messege(Message message,vector<Node> recipients);
    void send_messege(Message message,Address *toaddr);

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();
    int get_node_location_in_ring();

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

    // Other handler
    void create_replica_to_next(Node toNode, ReplicaType repType);
    void create_replica_to_prev(Node toNode, ReplicaType repType);

    // Read repair handler
    void update_to_latest_value(int transaction_id, int time_stamp, string value);
    string get_latest_value(int transaction_id);

    // Quorum handler
    void set_quorum_count(int transaction_id, int time_stamp, int quorum_count, MessageType message_type, string key, string value);
    void reduce_quorum_count(int transaction_id, int quorum_count);
    bool if_quorum_count_met(int transaction_id);
    void remove_transaction_entry(int tranaction_id);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	~MP2Node();
};

static char* ReplicaType_str(ReplicaType t) {
    if (t == ReplicaType::PRIMARY) return "PRIMARY";
    if (t == ReplicaType::SECONDARY) return "SECONDARY";
    if (t == ReplicaType::TERTIARY) return "TERTIARY";
    return "UNKNOWN_TYPE";
}

// MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY}
static char* MessageType_str(MessageType t) {
    if (t == MessageType::CREATE) return "CREATE";
    if (t == MessageType::READ) return "READ";
    if (t == MessageType::UPDATE) return "UPDATE";
    if (t == MessageType::DELETE) return "DELETE";
    if (t == MessageType::REPLY) return "REPLY";
    if (t == MessageType::READREPLY) return "READREPLY";
    return "UNKNOWN_TYPE";
}


#define MY_DEBUGLOG2 1

#if MY_DEBUGLOG2 == 1
static void printAddress(Address *addr) {
    printf("%d.%d.%d.%d:%d", addr->addr[0], addr->addr[1], \
                             addr->addr[2], addr->addr[3], *(short*)&addr->addr[4]);
}

#define DPRINT(...) do{                                             \
    printf("\n[");                                                  \
    printAddress(&memberNode->addr);                                \
    printf("][%d][%s:%d]",par->getcurrtime(),__func__,__LINE__);    \
    printf(__VA_ARGS__);                                            \
}while(0)
#else /* MY_DEBUGLOG2 */
#define DPRINT(...)
#endif /* MY_DEBUGLOG2 */

#endif /* MP2NODE_H_ */
