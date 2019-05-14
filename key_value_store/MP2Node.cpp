/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
    this->init11 = false;
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: send_messege
 *
 * DESCRIPTION: Send messege to multiple nodes.
 * int EmulNet::ENsend(Address *myaddr, Address *toaddr, char *data, int size)
 */
void MP2Node::send_messege(Message message, vector<Node> recipients){
    Address* src_addr = &(this->memberNode->addr);
    // string str_rep = message.toString();
    // char * msg_str_c = (char*)str_rep.c_str();
    size_t msg_len = message.toString().length();
    char *msg_str_c = (char*) malloc(msg_len * sizeof(char));
    strcpy(msg_str_c, message.toString().c_str());
    DPRINT("Messege %s of length %d", msg_str_c, msg_len);
    for (size_t i = 0; i < recipients.size(); ++i) {
        Address *dest_addr = recipients[i].getAddress();
        DPRINT("Sending Messege to [%s]", dest_addr->addr);
        // this->emulNet->ENsend(src_addr, &(recipients[i].nodeAddress), msg_str_c, msg_len);
        this->emulNet->ENsend(src_addr, dest_addr, msg_str_c, msg_len);
    }
}

/**
 * FUNCTION NAME: send_messege
 *
 * DESCRIPTION: Send messege to multiple nodes.
 * int EmulNet::ENsend(Address *myaddr, Address *toaddr, char *data, int size)
 */
void MP2Node::send_messege(Message message, Address *toaddr){
    Address* src_addr = &(this->memberNode->addr);
    // string str_rep = message.toString();
    // char * msg_str_c = (char*)str_rep.c_str();
    // char * msg_str_c = (char*)(message.toString()).c_str();
    size_t msg_len = message.toString().length();
    char *msg_str_c = (char*) malloc(msg_len * sizeof(char));
    strcpy(msg_str_c, message.toString().c_str());
    // DPRINT("Messege %s---%s of length %d",message.toString().c_str(),*msg_str_c,msg_len);
    DPRINT("Messege %s of length %d", msg_str_c, msg_len);
    DPRINT("Sending Messege to [%s]", toaddr->addr);
    this->emulNet->ENsend(src_addr, toaddr, msg_str_c, msg_len);
    free(msg_str_c);
}

/**
 * FUNCTION NAME: update_to_latest_value
 *
 * DESCRIPTION: Hold the value with latest time stamp for the key-transaction id
 */
void MP2Node::update_to_latest_value(int transaction_id, int time_stamp, string value) {
    if (time_stamp > this->value_ts_map[transaction_id].time_stamp && \
        !(this->value_ts_map[transaction_id].value == value)) {
        // Need read repair
        this->value_ts_map[transaction_id].time_stamp = time_stamp;
        this->value_ts_map[transaction_id].value = value;
        this->value_ts_map[transaction_id].read_repair_needed = true;
        DPRINT("UPDATED read timestamp for transaction id %d with value %s", transaction_id, value.c_str());
    }
}

/**
 * FUNCTION NAME: get_latest_value
 *
 * DESCRIPTION: Return the value with latest time stamp for the key-transaction id
 */
string MP2Node::get_latest_value(int transaction_id) {
    return this->value_ts_map[transaction_id].value;
}

/**
 * FUNCTION NAME: set_quorum_count
 *
 * DESCRIPTION: Set the initial quorum count that should be met for the specified transaction.
 */
void MP2Node::set_quorum_count(int transaction_id, int time_stamp, int quorum_count, MessageType message_type, string key, string value) {
    DPRINT("Adding <%d : <%d, %s, %s, %s>> in transaction_quorum", transaction_id, quorum_count, MessageType_str(message_type), key.c_str(), value.c_str());
    // pair <int, MessageType> pr1 (quorum_count, message_type);
    // pair <int, pair <int, MessageType>> pr2 (transaction_id, pr1);
    TransactionEntry tr = {time_stamp, quorum_count, message_type, key, value};
    this->transaction_quorum.insert(pair<int, TransactionEntry> (transaction_id, tr));
    // this->transaction_quorum.insert(transaction_id, quorum_count);  // TODO try emplace
}

/**
 * FUNCTION NAME: reduce_quorum_count
 *
 * DESCRIPTION: Reduce the quorum count if success response is received for the specified transaction.
 */
void MP2Node::reduce_quorum_count(int transaction_id, int quorum_count) {
    DPRINT("Reducing quorum count of transaction_id- %d by %d", transaction_id, quorum_count);
    this->transaction_quorum[transaction_id].quorum_count = this->transaction_quorum[transaction_id].quorum_count - quorum_count;
    DPRINT("Reduced quorum count of transaction_id- %d to %d", transaction_id, this->transaction_quorum[transaction_id].quorum_count);
}

/**
 * FUNCTION NAME: if_quorum_count_met
 *
 * DESCRIPTION: Return true if quorum count is achieved.
 */
bool MP2Node::if_quorum_count_met(int transaction_id) {
    if (this->transaction_quorum[transaction_id].quorum_count == 0) {
        DPRINT("Quorum count achieved for transaction_id- %d is %d", transaction_id, this->transaction_quorum[transaction_id].quorum_count);
        return true;
    } else {
        return false;
    }

}

/**
 * FUNCTION NAME: remove_transaction_entry
 *
 * DESCRIPTION: Return true if quorum count is achieved.
 */
void MP2Node::remove_transaction_entry(int transaction_id) {
    if (this->transaction_quorum[transaction_id].quorum_count <= 0) {
        DPRINT("Removing entry for transaction_id- %d", transaction_id);
        this->transaction_quorum.erase(transaction_id);
        DPRINT("Removed entry for transaction_id- %d", transaction_id);
    }
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() { // Implement it
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());

    // for(auto i : curMemList) {
    //     DPRINT("== %s --- %u", i.nodeAddress.getAddress().c_str(), i.getHashCode());
    // }

    // Compare this->ring and curMemList - if different then call this->stabilizationProtocol()
    if (this->ring.size() != curMemList.size()) {
        DPRINT("The ring size has changed. Stablization needs to be done.");
        change = true;
    }
    /**
       else {
        // Check if the all the members are equal
        for (int i = 0 ; i < this->ring.size(); i++) {
            if (!(this->ring[i].nodeAddress == curMemList[i].nodeAddress)) {
                change = true;
                DPRINT("Ring node mismatch.")
                break;
            }
        }
    }    */

    this->ring = curMemList;
	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
    if (this->ht->currentSize() > 0 && change){
        // DPRINT("The ring has changed. Calling stabilizationProtocol.");
        this->stabilizationProtocol();
    } else {
        // DPRINT("The ring has not changed or hash-table size is not greater that zero. Not calling stabilizationProtocol.");
    }

    // TODO Remove
    // string s("**");
    // for(auto i : this->ring) {
    //     s = s + "---" + i.nodeAddress.getAddress();
    // }
    // DPRINT("-------------------------------------");
    // DPRINT("[RING] %s", s.c_str());
    // DPRINT("-------------------------------------");
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) { // Implement it
	/*
	 * Implement this
	 */

    /**
     *
     * 1. Identify the replica-id using hashFunction() and findNodes() for ReplicaType - {PRIMARY, SECONDARY, TERTIARY}
     * 2. For three replicas send messege to the server with MessageType = CREATE along with the key-value pair
     * Message(int _transID, Address _fromAddr, MessageType _type, string _key, string _value, ReplicaType _replica);
     *
     *
     */
    g_transID++;
    vector<Node> recipients = findNodes(key);
    for (auto i : recipients) {
        std::cout<<"---------resipients--"<<i.nodeAddress.getAddress();
    }
    // assert(recipients.size()==NUM_OF_REPLICAS);  // TODO remove
    // Address* sendaddr = &(this->memberNode->addr);

    // Set quorum info
    this->set_quorum_count(g_transID, par->getcurrtime(), QUORUM_COUNT, MessageType::CREATE, key, value);
    DPRINT("---------------CREATE------------------");
    DPRINT("Send CREATE request {%s : %s}", key.c_str(), value.c_str());
    for (int i = 0; i < NUM_OF_REPLICAS; i++) {
        Message create_msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value, static_cast<ReplicaType>(i));
        DPRINT("[ND]---");
        send_messege(create_msg, recipients[i].getAddress());
    }
    DPRINT("CREATE request {%s : %s} sent", key.c_str(), value.c_str());

    // TODO Check for Quorum

}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){ // Implement it
	/*
	 * Implement this
	 */
    g_transID++;
    vector<Node> recipients = findNodes(key);
    // std::random_shuffle(recipients.begin(), recipients.end());
    // Set quorum info
    DPRINT("@@@@@@@@@@@@@ transaction id %d --- key [%s]", g_transID, key.c_str());
    this->set_quorum_count(g_transID, par->getcurrtime(), QUORUM_COUNT, MessageType::READ, key, "READ_DUMMY");
    DPRINT("Send READ request for transaction %d --- key %s", g_transID, key.c_str());
    Message create_msg(g_transID, this->memberNode->addr, MessageType::READ, key);
    // Send read request to quorum number of random nodes
    // for(int i = 0; i < QUORUM_COUNT; i++) {
    //    send_messege(create_msg, recipients[i].getAddress());
    // }
    send_messege(create_msg, recipients);
    DPRINT("READ request for key %s sent", key.c_str());
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){ // Implement it
	/*
	 * Implement this
	 */
    g_transID++;
    vector<Node> recipients = findNodes(key);
    // Set quorum info
    this->set_quorum_count(g_transID, par->getcurrtime(), QUORUM_COUNT, MessageType::UPDATE, key, value);

    DPRINT("Send UPDATE request {%s : %s}", key.c_str(), value.c_str());
    for (int i = 0; i < NUM_OF_REPLICAS; i++) {
        Message create_msg(g_transID, this->memberNode->addr, MessageType::UPDATE, key, value, static_cast<ReplicaType>(i));
        send_messege(create_msg, recipients[i].getAddress());
    }
    DPRINT("UPDATE request {%s : %s} sent", key.c_str(), value.c_str());


    // TODO Check for Quorum

}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){ // Implement it
	/*
	 * Implement this
	 */
    if (key == INVALID_KEY) {
        DPRINT("Got and invalid key client %s", key.c_str());
        // return;
    }

    g_transID++;
    vector<Node> recipients = findNodes(key);
    // assert(recipients.size()==NUM_OF_REPLICAS);  // TODO remove
    // Set quorum info
    this->set_quorum_count(g_transID, par->getcurrtime(), QUORUM_COUNT, MessageType::DELETE, key, string("DELETE_DUMMY"));

    DPRINT("Send DELETE request {%s : %s}", key.c_str());
    Message create_msg(g_transID, this->memberNode->addr, MessageType::DELETE, key);
    send_messege(create_msg, recipients);
    DPRINT("DELETE request {%s : %s} sent", key.c_str());

    // TODO Check for Quorum

}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) { // Implement it
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
    // It should store <string key, Entry(string value, int timestamp, ReplicaType replica)
    bool ret = true;
    Entry item(value, par->getcurrtime(), replica);
    string item_str(item.convertToString());
    DPRINT("HashTable Entry [%s]", item_str.c_str());
    ret = this->ht->create(key, item_str);
    DPRINT("Created HashTable Entry [%s]", item_str.c_str());
    return ret;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) { // Implement it
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value

    string value;
    DPRINT("READING %s from hash table", key.c_str());
    value = this->ht->read(key);  // Value if found else ""
    DPRINT("Read HashTable Entry %s. Value %s", key.c_str(), value.c_str());
    return value;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) { // Implement it
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false

    bool ret = true;
    Entry item(value, par->getcurrtime(), replica);
    string item_str(item.convertToString());
    DPRINT("HashTable Entry [%s]", item_str.c_str());
    ret = this->ht->update(key, item_str);
    DPRINT("Updated HashTable Entry [%s]", item_str.c_str());
    return ret;
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) { // Implement it
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table

    bool ret = true;
    DPRINT("DELETING %s from hash table", key.c_str());
    ret = this->ht->deleteKey(key);
    DPRINT("Deleted HashTable Entry %s", key.c_str());
    return ret;
}

/**
 * FUNCTION NAME: dispatchMessages
 *
 * DESCRIPTION: coordinator dispatches messages to corresponding nodes
 */
void MP2Node::dispatchMessages(Message message){ // Implement my
    // TODO: Try to get the desination here. Then use it instead of send_messege()

}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 * Also take care of node failure where no response received and transaction times out.
 * From common.h
 * enum MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
 * enum of replica types
  * enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};
 */
void MP2Node::checkMessages() { // Implement it
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char *data;
	int size;

	/*
	 * Declare your local variables here
	 */
    bool ret = true;
    bool success = true;
    int trans_id = 0;  // TODO [Important] - Handle transaction ID
    string key;
    string value;
    MessageType message_type;
    int transaction_time_stamp;

    // Traverse transaction_quorum and check for timeout failure
    for ( auto tr : this->transaction_quorum) {
        if (par->getcurrtime() - tr.second.time_stamp > RESPONSE_WAIT_TIME) {
            // Remove the entry print failure log
            key = tr.second.key;
            value = tr.second.value;
            trans_id = tr.first;
            DPRINT("transaction id %d --- key %s --- MessageType %s", trans_id, key.c_str(), MessageType_str(tr.second.message_type));
            switch (tr.second.message_type) {
                case MessageType::CREATE:
                    DPRINT("Create response timeout");
                    DPRINT("memberNode [%p]", memberNode);
                    DPRINT("memberNode->addr [%s]", memberNode->addr.getAddress().c_str());
                    DPRINT("&memberNode->addr [%p]", &memberNode->addr);
                    DPRINT("---");
                    log->logCreateFail(&memberNode->addr, true, trans_id, key, value);
                    break;
                case MessageType::UPDATE:
                    DPRINT("Update response timeout");
                    log->logUpdateFail(&memberNode->addr, true, trans_id, key, value);
                    break;
                case MessageType::READ:
                    DPRINT("Read response timeout");
                    log->logReadFail(&memberNode->addr, true, trans_id, key);
                    break;
                case MessageType::DELETE:
                    DPRINT("Delete response timeout");
                    log->logDeleteFail(&memberNode->addr, true, trans_id, key);
                    break;
                default:
                    break;
            }
            DPRINT("Removing transaction id-%d from transaction_quorum", trans_id);
            this->transaction_quorum.erase(trans_id);
        }
    }

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);
        Message rcvd_msg(message);
        trans_id = rcvd_msg.transID;
        DPRINT("Received messege %s", rcvd_msg.toString().c_str());
		/*
		 * Handle the message types here
         * MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
		 */

        /**
         * if messegetype == CREATE then call bool MP2Node::createKeyValue(string key, string value, ReplicaType replica)
         *
         * Process the messege and send respose.
         * Response messge format - Message(int _transID, Address _fromAddr, MessageType _type, bool _success);
         */

        // server side rcvd_msg handling
        switch((MessageType)rcvd_msg.type){
            case MessageType::CREATE:
                DPRINT("CREATE key-value entry in node with replica %d for key %s",rcvd_msg.replica,rcvd_msg.key.c_str());
                ret = this->createKeyValue(rcvd_msg.key, rcvd_msg.value, rcvd_msg.replica);
                if(ret){
                    success = true;
                    DPRINT("Created key-value entry in node with replica %d for key %s",rcvd_msg.replica,rcvd_msg.key.c_str());
                    log->logCreateSuccess(&memberNode->addr, false, trans_id, rcvd_msg.key, rcvd_msg.value);
                }else{
                    success = false;
                    DPRINT("Error while creating key-value entry in node with replica %d for key %s", rcvd_msg.replica, rcvd_msg.key.c_str());
                    log->logCreateFail(&memberNode->addr, false, trans_id, rcvd_msg.key, rcvd_msg.value);
                }
                {
                    Message reply_msg(trans_id, (this->memberNode->addr), MessageType::REPLY, success);
                    send_messege(reply_msg, &rcvd_msg.fromAddr);
                }
                break;

            case MessageType::UPDATE:
                DPRINT("UPDATE key-value entry in node with replica %d for key %s",rcvd_msg.replica,rcvd_msg.key.c_str());
                ret = this->updateKeyValue(rcvd_msg.key, rcvd_msg.value, rcvd_msg.replica);
                if(ret){
                    success = true;
                    DPRINT("Updated key-value entry in node with replica %d for key %s",rcvd_msg.replica,rcvd_msg.key.c_str());
                    log->logUpdateSuccess(&memberNode->addr, false, trans_id, rcvd_msg.key, rcvd_msg.value);
                }else{
                    success = false;
                    DPRINT("Error while updating key-value entry in node with replica %d for key %s", rcvd_msg.replica, rcvd_msg.key.c_str());
                    log->logUpdateFail(&memberNode->addr, false, trans_id, rcvd_msg.key, rcvd_msg.value);
                }
                {
                    Message reply_msg(trans_id, (this->memberNode->addr), MessageType::REPLY, success);
                    send_messege(reply_msg, &rcvd_msg.fromAddr);
                }
                break;

            case MessageType::DELETE:
                DPRINT("DELETE key-value entry in node for key %s", rcvd_msg.key.c_str());
                ret = this->deletekey(rcvd_msg.key);
                if(ret){
                    success = true;
                    DPRINT("Deleted key-value entry in node for key %s", rcvd_msg.key.c_str());
                    log->logDeleteSuccess(&memberNode->addr, false, trans_id, rcvd_msg.key);
                }else{
                    success = false;
                    DPRINT("Error while deleting key-value entry in node for key %s", rcvd_msg.key.c_str());
                    log->logDeleteFail(&memberNode->addr, false, trans_id, rcvd_msg.key);
                }
                {
                    Message reply_msg(trans_id, (this->memberNode->addr), MessageType::REPLY, success);
                    send_messege(reply_msg, &rcvd_msg.fromAddr);
                }
                break;

            case MessageType::READ:
                DPRINT("READ key-value entry in node for key %s", rcvd_msg.key.c_str());
                value = this->readKey(rcvd_msg.key);
                // if(!value.empty()){
                if(value != string("")){
                    success = true;
                    DPRINT("Successfully Read key-value entry in node for key %s value %s", rcvd_msg.key.c_str(), value.c_str());
                    log->logReadSuccess(&memberNode->addr, false, trans_id, rcvd_msg.key, value);
                }else{
                    success = false;
                    DPRINT("Error while reading key-value entry in node for key %s", rcvd_msg.key.c_str());
                    log->logReadFail(&memberNode->addr, false, trans_id, rcvd_msg.key);
                }
                {
                    // construct read reply message
                    // Message::Message(int _transID, Address _fromAddr, string _value)
                    Message reply_msg(trans_id, (this->memberNode->addr), value); // The value is of Entry type
                    send_messege(reply_msg, &rcvd_msg.fromAddr);
                }
                break;
            // client side rcvd_msg handling
            case MessageType::READREPLY:
                // 1. Response with value with latest timestamp
                // 2. Read Repair
                {
                    message_type = this->transaction_quorum[trans_id].message_type;
                    key = this->transaction_quorum[trans_id].key;
                    // value = this->transaction_quorum[trans_id].value;
                    transaction_time_stamp = this->transaction_quorum[trans_id].time_stamp;
                    DPRINT("key [%s]", key.c_str());
                    DPRINT("rcvd_msg [%s]", rcvd_msg.toString().c_str());
                    DPRINT("rcvd_msg.value [%s]",rcvd_msg.value.c_str());
                    DPRINT("####");
                    // Entry val(rcvd_msg.value);
                    // DPRINT("####");
                    // int value_time_stamp = val.timestamp;
                    // value = val.value;  // Actual string value

                    bool need_read_repair = false;
                    DPRINT("par->getcurrtime() %d --- transaction_time_stamp %d", par->getcurrtime(), transaction_time_stamp);
                    if (rcvd_msg.value != string("") && (par->getcurrtime() - transaction_time_stamp <= RESPONSE_WAIT_TIME)) {
                        Entry val(rcvd_msg.value);
                        DPRINT("#### reduce_quorum_count");
                        int value_time_stamp = val.timestamp;
                        value = val.value;  // Actual string value
                        this->reduce_quorum_count(trans_id, 1);
                        this->update_to_latest_value(trans_id, value_time_stamp, value);
                        if (this->if_quorum_count_met(trans_id) == true) {
                            value = this->get_latest_value(trans_id);
                            DPRINT("READ quorum achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                            log->logReadSuccess(&memberNode->addr, true, trans_id, key, value);
                            need_read_repair = this->value_ts_map[trans_id].read_repair_needed;
                            this->value_ts_map.erase(trans_id);
                            this->remove_transaction_entry(trans_id);
                            this->clientUpdate(key, value); // Update all the entries with latest value
                        }
                    } else {
                        // Key not present or timeout
                        if (par->getcurrtime() - transaction_time_stamp <= RESPONSE_WAIT_TIME) {
                            DPRINT("RESPONSE TIME OUT for transaction id %d --- key %s", trans_id, key);
                        }
                        this->reduce_quorum_count(trans_id, 1);
                        if (this->if_quorum_count_met(trans_id) == true) {
                            DPRINT("READ quorum not achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                            log->logReadFail(&memberNode->addr, true, trans_id, key);
                            this->value_ts_map.erase(trans_id);
                            this->remove_transaction_entry(trans_id);
                        }
                    }
                }
                break;
            case MessageType::REPLY:
                // REPLY message recived in response to CREATE, UPDATE, DELETE requests
                // Check if responae is a success
                // Update the quorum count
                // If quorum count met then log success for the transaction
                message_type = this->transaction_quorum[trans_id].message_type;
                key = this->transaction_quorum[trans_id].key;
                value = this->transaction_quorum[trans_id].value;
                transaction_time_stamp = this->transaction_quorum[trans_id].time_stamp;

                if (rcvd_msg.success == true && (par->getcurrtime() - transaction_time_stamp <= RESPONSE_WAIT_TIME)) {
                    this->reduce_quorum_count(trans_id, 1);
                    if (this->if_quorum_count_met(trans_id) == true) {
                        // Log success bassed on MessageType
                        switch(message_type) {
                            case MessageType::CREATE:
                                DPRINT("CREATE quorum achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logCreateSuccess(&memberNode->addr, true, trans_id, key, value);
                                break;
                            case MessageType::UPDATE:
                                DPRINT("UPDATE quorum achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logUpdateSuccess(&memberNode->addr, true, trans_id, key, value);
                                break;
                            case MessageType::READ:
                                DPRINT("READ quorum achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logReadSuccess(&memberNode->addr, true, trans_id, key, value);
                                break;
                            case MessageType::DELETE:
                                DPRINT("DELETE quorum achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logDeleteSuccess(&memberNode->addr, true, trans_id, key);
                                break;
                            default:
                                break;
                        }
                        // DPRINT("&&&&&&&&&&&&&&&&&&&");
                        this->remove_transaction_entry(trans_id);
                    }
                } else {
                    // rcvd_msg.success == false
                    if (par->getcurrtime() - transaction_time_stamp <= RESPONSE_WAIT_TIME) {
                        DPRINT("RESPONSE TIME OUT for transaction id %d --- key %s", trans_id, key);
                    }
                    this->reduce_quorum_count(trans_id, 1);
                    if (this->if_quorum_count_met(trans_id) == true) {
                        switch(message_type) {
                            case MessageType::CREATE:
                                DPRINT("CREATE quorum not achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logCreateFail(&memberNode->addr, true, trans_id, key, value);
                                break;
                            case MessageType::UPDATE:
                                DPRINT("UPDATE quorum not achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logUpdateFail(&memberNode->addr, true, trans_id, key, value);
                                break;
                            case MessageType::READ:
                                DPRINT("**READ quorum not achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logReadFail(&memberNode->addr, true, trans_id, key);
                                break;
                            case MessageType::DELETE:
                                DPRINT("DELETE quorum not achieved for transaction ID %d {%s : %s}", trans_id, key.c_str(), value.c_str());
                                log->logDeleteFail(&memberNode->addr, true, trans_id, key);
                                break;
                            default:
                                break;
                        }
                        this->remove_transaction_entry(trans_id);
                    }
                }
                break;
        }

	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

int MP2Node::get_node_location_in_ring() { // Implement my
    int i = 0;
    for (i = 0; i < this->ring.size(); i ++) {
        if (*ring[i].getAddress() == this->memberNode->addr) {
            return i;
        }
    }
}

/**
 * FUNCTION NAME: create_replica_to_next
 *
 * DESCRIPTION: Rplication handler
 *              1. Search the PRIMARY replica.
 *              2. Get the key, value pair.
 *              3. Send the CREATE messege to the destination node.
 */
void MP2Node::create_replica_to_next(Node dest_node, ReplicaType dest_replica_type) { // Implement my
#if 0
    string key;
    string value;
    for (auto i : this->ht->hashTable) {
        string k = i.first;
        string v = i.second;
        Entry e(v);
        if (e.replica == ReplicaType::PRIMARY) { // dest_replica_type) {
            key = k;
            value = v;
            break;
        }
    }
    g_transID++;
    DPRINT("Replicate [%s] {%s : %s} %s", dest_node.nodeAddress.getAddress().c_str(), key.c_str(), value.c_str(), ReplicaType_str(dest_replica_type));
    Message replicate_to_msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value, dest_replica_type);  // TODO: create or update??
    send_messege(replicate_to_msg, dest_node.getAddress());
    ///////////
#else
    string key;
    string value;
    for (auto i : this->ht->hashTable) {
        string k = i.first;
        string v = i.second;
        Entry e(v);
        if (e.replica == ReplicaType::PRIMARY) { // dest_replica_type) {
            key = k;
            value = v;
            g_transID++;
            DPRINT("Replicate [%s] {%s : %s} %s", dest_node.nodeAddress.getAddress().c_str(), key.c_str(), value.c_str(), ReplicaType_str(dest_replica_type));
            Message replicate_to_msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value, dest_replica_type);  // TODO: create or update??
            send_messege(replicate_to_msg, dest_node.getAddress());
        }
    }
    // g_transID++;
    // DPRINT("Replicate [%s] {%s : %s} %s", dest_node.nodeAddress.getAddress().c_str(), key.c_str(), value.c_str(), ReplicaType_str(dest_replica_type));
    // Message replicate_to_msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value, dest_replica_type);  // TODO: create or update??
    // send_messege(replicate_to_msg, dest_node.getAddress());
#endif
}

/**
 * FUNCTION NAME: create_replica_to_prev
 *
 * DESCRIPTION: Rplication handler
 *              1. Delete the Entry in hash table of replica_type.
 *              2. Ask the dest_node for replica_type messege.
 */
void MP2Node::create_replica_to_prev(Node dest_node, ReplicaType src_replica_type) { // Implement my
    string key;
    string value;
    for (auto i : this->ht->hashTable) {
        string k = i.first;
        string v = i.second;
        Entry e(v);
        if (e.replica == src_replica_type) {
            key = k;
            value = v;
            g_transID++;
            DPRINT("Replicate [%s] {%s : %s} %s", dest_node.nodeAddress.getAddress().c_str(), key.c_str(), value.c_str(), ReplicaType_str(ReplicaType::PRIMARY));
            Message replicate_to_msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value, ReplicaType::PRIMARY);  // TODO: create or update??
            send_messege(replicate_to_msg, dest_node.getAddress());
        }
    }
    // DPRINT("DELETING src_replica_type %s key %s", ReplicaType_str(src_replica_type), key.c_str());
    // this->ht->deleteKey(key); // TODO Check

    // TODO : Ask for the key-value pair for the src_replica_type
}

#if 1
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() { // Implement it
	/*
	 * Implement this
	 */
    if (this->memberNode->bFailed == false) {
        int node_index;
        int prev_2, prev_1;
        int next_2, next_1;
        auto ring_size = this->ring.size();

        node_index = this->get_node_location_in_ring();
        DPRINT("**************&&*******");
        // Identify the neighbours who replica it should
        // DPRINT("[ND] node_index %d", node_index);
        prev_2 = ((node_index - 2) < 0 ? node_index - 2 + ring_size : node_index - 2) % ring_size;
        prev_1 = ((node_index - 1) < 0 ? node_index - 1 + ring_size : node_index - 1) % ring_size;

        // Identify the neighbours who should contain its replica
        next_1 = (node_index + 1) % ring_size;
        next_2 = (node_index + 2) % ring_size;
        // DPRINT("%d---%d---%d---%d---%d", prev_2, prev_1, node_index, next_1, next_2);
        // DPRINT("---");
        // if (this->memberNode->bFailed == false) {
        //     DPRINT("--TEST-");
        //     DPRINT("---");
        //     // DPRINT("---hasMyReplicas size %s", this->hasMyReplicas.size());
        //     // DPRINT("---haveReplicasOf size %s", this->haveReplicasOf.size());
        // }
        // DPRINT("---");
        if (this->init11) {
        // if (ring_size > 0 && this->memberNode->bFailed == false && !this->hasMyReplicas.empty() && !this->haveReplicasOf.empty()) {
            // string s("**");
            // for(auto i : this->ring) {
            //     s = s + "---" + i.nodeAddress.getAddress();
            // }
            // DPRINT("-------------------------------------");
            // DPRINT("[RING] %s", s.c_str());
            // DPRINT("-------------------------------------");
            DPRINT("ring_size %d", ring_size);
            DPRINT("Neighbours entries [[%s-%s]-%s-[%s-%s]]", ring[prev_2].nodeAddress.getAddress().c_str(),
                                                            ring[prev_1].nodeAddress.getAddress().c_str(),
                                                            ring[node_index].nodeAddress.getAddress().c_str(),
                                                            ring[next_1].nodeAddress.getAddress().c_str(),
                                                            ring[next_2].nodeAddress.getAddress().c_str());
            Node n1 = ring[next_1]; // next node
            Node n2 = ring[next_2]; // next of next node
            Node p1 = ring[prev_1]; // previous node
            Node p2 = ring[prev_2]; // previous of previous node
            // DPRINT("[ND] --- n1 %u --- hasMyReplica %u", n1.getHashCode(), this->hasMyReplicas[0].getHashCode());
            // DPRINT("[ND] --- n1 %s", n1.nodeAddress.getAddress().c_str());
            // // DPRINT("[ND] --- this->hasMyReplicas %s", n1.nodeAddress.getAddress().c_str());

            // DPRINT("---");
            // DPRINT("---hasMyReplicas size %s", this->hasMyReplicas.size());
            // DPRINT("---haveReplicasOf size %s", this->haveReplicasOf.size());
            // DPRINT("---");
            // DPRINT("1-%d --- 2-%d",this->hasMyReplicas[0].nodeHashCode, this->hasMyReplicas[1].nodeHashCode);

            if (!(n1.nodeAddress == this->hasMyReplicas[0].nodeAddress)) {
                /**
                 * The next node in the ring that was containing the SECONDARY replica has failed.
                 * Replicate PRIMARY to SECONDARY of n1
                 */

                DPRINT("Replicating %s replica to [%s]", ReplicaType_str(ReplicaType::SECONDARY), n1.nodeAddress.getAddress().c_str());
                create_replica_to_next(n1, ReplicaType::SECONDARY);
            }
            if (!(n2.nodeAddress == this->hasMyReplicas[1].nodeAddress)) {
                /**
                 * The next of next node in the ring that was containing the TERTIARY replica has failed.
                 * Replicate PRIMARY to TERTIARY of n2
                 */
                DPRINT("Replicating %s replica to [%s]", ReplicaType_str(ReplicaType::TERTIARY), n2.nodeAddress.getAddress().c_str());
                create_replica_to_next(n2, ReplicaType::TERTIARY);
            }
            // if (!(p1.nodeAddress == this->haveReplicasOf[0].nodeAddress)) {
            //     /**
            //      * The previous node in the ring has failed.
            //      * Delete the SECONDARY replica to create space for replication p1's PRIMARY to SECONDARY here.
            //      */
            //     DPRINT("Replicating %s of this to PRIMARY of replica of [%s]", ReplicaType_str(ReplicaType::SECONDARY), p1.nodeAddress.getAddress().c_str());
            //     create_replica_to_prev(p1, ReplicaType::SECONDARY);
            //     // create_replica_to_next(p1, ReplicaType::SECONDARY);

            // }
            // if (!(p2.nodeAddress == this->haveReplicasOf[1].nodeAddress)) {
            //     /**
            //      * The previous node in the ring has failed.
            //      * Delete the TERTIARY replica to create space for replication p2's PRIMARY here TERTIARY here.
            //      */
            //     DPRINT("Replicating %s of this to PRIMARY of replica of [%s]", ReplicaType_str(ReplicaType::TERTIARY), p2.nodeAddress.getAddress().c_str());
            //     create_replica_to_prev(p2, ReplicaType::TERTIARY);
            //     // create_replica_to_next(p2, ReplicaType::TERTIARY);

            // }
            // DPRINT("---");
            // DPRINT("---");
            // // this->haveReplicasOf.clear(); //
            // DPRINT("---");
            // DPRINT("---");
            this->hasMyReplicas.clear();
            DPRINT("---");
            DPRINT("---");
        } else {
            DPRINT("--- init11 = true");
            this->init11 = true;
        }
        // this->haveReplicasOf.push_back(ring[prev_2]); //
        // this->haveReplicasOf.push_back(ring[prev_1]); //
        this->hasMyReplicas.push_back(ring[next_1]);
        this->hasMyReplicas.push_back(ring[next_2]);
        DPRINT("<><><><><>Neighbours added [[%s-%s]-%s-[%s-%s]]", ring[prev_2].nodeAddress.getAddress().c_str(),
                                                        ring[prev_1].nodeAddress.getAddress().c_str(),
                                                        ring[node_index].nodeAddress.getAddress().c_str(),
                                                        ring[next_1].nodeAddress.getAddress().c_str(),
                                                        ring[next_2].nodeAddress.getAddress().c_str());
    }
}
#else
void MP2Node::stabilizationProtocol()
{
	/*
	 * Implement this
	 */

	if (!memberNode->bFailed)
	{
		//find size of hash table for the node and iterate through all the keys to recreate 3 replicas
		size_t htSize = this->ht->currentSize();
        map<string, string>::iterator hashItr;
		for (hashItr = this->ht->hashTable.begin(); hashItr != this->ht->hashTable.end(); hashItr++)
		{
			string keyStr = hashItr->first;
			Entry valEntry(hashItr->second);
			string valStr = valEntry.value;
			clientCreate(keyStr, valStr);
		}

		// delete valEntry;
	}
}
#endif