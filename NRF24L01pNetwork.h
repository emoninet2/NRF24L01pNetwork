/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NRF24L01pNetwork.h
 * Author: emon
 *
 * Created on September 7, 2017, 9:53 AM
 */

#ifndef NRF24L01PNETWORK_H
#define NRF24L01PNETWORK_H

#include <inttypes.h>
#include <stdint.h>
#include <limits>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

#include "NRF24L01p/NRF24L01p.h"

#define NRF24L01P_NETWORK_BROADCAST_ADDR 0x0526785439
#define NODE_PIPE_MASK 0xC0
#define ROUTING_TABLE_SIZE 20



#define NRF24L01P_NETWORK_PACKETCTRL_REQACK_BP 0
#define NRF24L01P_NETWORK_PACKETCTRL_REQACK_BM (1<<0)


class NRF24L01pNetwork : public NRF24L01p{
public:
    NRF24L01pNetwork();
    NRF24L01pNetwork(const NRF24L01pNetwork& orig);
    virtual ~NRF24L01pNetwork();
    
    
    /**
     * The enum containing the types of possible errors
     */
    typedef enum{
        SUCCESS = 0, /**< succes operation */
        ERROR = -1,  /**< failed operation */ 
                
    }NetworkErrorStatus_t;
    
    
    
    
    
    
    /** the header of the network payload
     * The header is encapsulated in the payload data. See networkPayload_t 
     */
    typedef struct {
        uint16_t srcNodeId; /**< two byte node id of the source node*/
        uint16_t destNodeId;/**< two byte node id of the destination node*/
        uint8_t pid;        /**< the packet identification of the network payload*/
        uint8_t packetInfo; /**< the other information of the packet*/
        uint8_t length;     /**< the length of the data (maximum network payload data that can be sent is 25 bytes) */
    }networkPayloadHeader_t;
    
    /** the structure of the network payload
     *  the structure is the actual 32 bytes containing the header information and the actual data.
     */
    typedef struct {
        networkPayloadHeader_t Header;
        uint16_t srcNodeId; /**< two byte node id of the source node*/
        uint16_t destNodeId;/**< two byte node id of the destination node*/
        uint8_t pid;        /**< the packet identification of the network payload*/
        uint8_t packetCtrl; /**< the other information of the packet*/
        uint8_t length;     /**< the length of the data (maximum network payload data that can be sent is 25 bytes) */
        uint8_t payload[25];/**< tge 25 byte data that is left over to send over the 32 byte link layer payload of the radio */
    }networkPayload_t;
    
    
    
    /** the enum containing the possible status of the adjacent nodes
     *  the status of the adjacent nodes as being free, connected or unresponsive. 
     */
    typedef enum{  
        AdjacentNodeStatus_Free = 0,
        AdjacentNodeStatus_Connected = 1,    
        AdjacentNodeStatus_Unresponsive = -1        
    }adjacentNodeStatus_t;
    
    
    /** structure of the adjacent node
     * the information needed for registering the adjacent noode is the node id , the pip that the adjacent node has dedicated, and the status
     */
    typedef struct{
        uint16_t nodeId;            /**< the node id of the adjacent node assigned by the coordinator*/
        pipe_t rxPipe;              /**< the pipe of the adjacent node that this node will send data to*/
        adjacentNodeStatus_t status;/**< the status of the adjacent node as being free, connected or unresponsive */
    }adjacentNode_t;
    
    /** structure of the routing data
     * the routing table consists of two fields. The destination node and the adjacent node that upstreams to the destination
     */
    typedef struct{
        uint16_t destNodeId;        /**< the destination node id*/
        adjacentNode_t viaAdjNode;  /**< the adjacent node that upstreams towards the destination node id */
    }routingNode_t;
    
    
    /**
     * placeholder for the node id of this node
     */
    uint16_t ownNodeId;
    /** 
     * placeholder for the network id this node connects to
     */
    uint16_t ownNetworkId;
    
    /** 
     * placeholder of the registered 5 friend nodes of this node
     */
    adjacentNode_t  AdjNode[5];
    
    /**
     * the routing table placeholder
     */
    routingNode_t     RoutingTable[ROUTING_TABLE_SIZE];
    
    /** 
     * the address at which the current router table points to
     */
    unsigned int RoutingTableAddr;
    
    /** 
     * if set, debug is enabled. 
     */
    bool NetDebugEnabled;
    
    
    /**initializes the network
     * this will initialize the network by defining its network and node id. If using DHCP, then this function is used by the DHC API layer.
     * @param networkId the 2 byte network id
     * @param nodeId the 2 byte node id
     */
    NetworkErrorStatus_t initNetwork(uint16_t networkId, uint16_t nodeId);
    
    /** function used to set another node as its adjacent node
     * this function uses the radios parallel pipe feature where each node assigns a dedicated pipe for an adjacent node.
     * @param AssignedPipe  the pipe of the adjacent node to which this node will send data to
     * @param adjNodeId     the node id of the node to be set as adjacent
     * @param AdjNodeRxPipe the nodes own pipe into which the adjacent node will send data to
     */
    NetworkErrorStatus_t setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe);
    /**remove an adjacent node
     * when an adjacent node is unresponsive, it can be removed using this function
     * @param AdjNodeRxPipe the pipe that has the unresponsive adjacent node egistered and needs to be removed
     */
    NetworkErrorStatus_t removeAdjacentNode(pipe_t AdjNodeRxPipe);
    /** send data to adjacent node
     * this function will send a network payload to and adjacent node
     * @param NetPayload the pointer to the network payload
     * @param AdjNode the pointer to the adjacent node to which the data needs to be sent 
     * @return error status, -1 means a fail and 0 means success
     */
    NetworkErrorStatus_t sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode);
    
    /** process the received network payload
     * this function will check the packet received and determine if it needs to be forwarded or kept for itself
     * this function will also update the routing table
     * @param payload the hardware payload containing informations relevant, such as rx pipe, etc. 
     */
    NetworkErrorStatus_t processNetworkPayload(Payload_t *payload);
    
    /** send data over the network
     * this function will send a data over the network. the routing table if aware of the upstream adjacent node will forward the data
     * the source node id will update the routing table of the intermittent nodes that the packet hops over
     * the network payload must contain the source and destination node id defined
     * @param pointer to the NetPayload the network payload that needs to be sent
     * @return returns the error status. -1 for fail and 0 for success
     */
    NetworkErrorStatus_t sendToNetwork(networkPayload_t *NetPayload);
    
    /**forward the packet 
     * this functon is used if the destination does not match this node's own node id
     * @param payload the payload (not network payload) is forwarded. No data has been modified. 
     * @return error status. -1 as fail and 0 as success
     */
    NetworkErrorStatus_t forwardPacket(Payload_t *payload);
    
    /**update the routing table
     * the routing table will check if the payload contains any new information and update the routing table accordingly
     * @param payload the payload that the radio receives and has to be forwarded.
     */
    NetworkErrorStatus_t routingTableUpdate(Payload_t *payload);
    
    /** sending an acknowledgement back
     * this acknowledgement packet is sent from the destination node back to the source node
     * @param payload the payload containing the acknowledgement 
     */
    NetworkErrorStatus_t sendAcknowledgement(Payload_t *payload);

    
    /**
     * 
     * @param payload
     * @param NetPayload
     */
    void packetEncapsulate(Payload_t *payload, networkPayload_t *NetPayload);
    /**
     * 
     * @param NetPayload
     * @param payload
     */
    void packetDecapsulate(networkPayload_t *NetPayload , Payload_t *payload);
    
    
    
    ////////DEBUGGING////////////
    
    
    void showAllAdjacentNodes(void);
    
    ///////////////DHC functions <tested by needs improvement and further testing> //////////////////////////
    
    
    /** The enum containing the Broadcast commands
     * The broadcast commands are sent to the broadcast TX adddress that is common in all the nodes in their pipe 0 RX address
     */
    typedef enum{
        GENERAL_BROADCAST = 0xC0,   /**< send a broadcast to all the nodes available*/
        GENERAL_CALL_REPLY = 0xC1,  /**< responses sent back from nodes that receives the general broadcast*/
        REPLY_GENERAL_CALL = 0xC2,  /**< the reply to a node that responses of it availablilty */
        PING_UID = 0xC3,            /**< used to send a ping */
        PONG_UID = 0xC4,            /**< used to reply to a ping*/
        REQUEST_CONNECTION = 0xC5,  /**< request connection to the network*/
        RESPOND_CONNECTION = 0xC6,  /**< respond connection to the network if its added, declined , etc */        
    }BroadcastCommand_t;
    
    /** structure of the registry table data
     * kept by the host client to map all the nodes in the network
     */
    typedef struct{
        uint16_t NodeId;        /**< the node id of a node in the network*/
        uint16_t friendNodeId;  /**< the adjacent node of this node that upstreams towards the coordinator*/
        uint32_t uid;           /**< the unique identification of the node*/
        uint8_t status;         /**< the status of the node as reported by its friend node*/
    }HostClients_t;
    
    /** structure of the broadcast payload
     * the broadcast payload is send on pipe 0 of all nodes. 
     */
    typedef struct{
        uint32_t srcUID;    /**< the UID of the node sending the broadcast packet*/
        uint32_t destUID;   /**< the UID of the destination node to which the broadcast packet needs to be sent to*/
        uint16_t NetworkID; /**< the node id of the network*/
        BroadcastCommand_t Cmd; /**< the broadcast command. See BroadCastCommand_t for the available commands*/
        uint8_t len;        /**< length of the broadcast payload*/ 
        uint8_t data[20];   /**< the remaining 20 bytes can be used to send other data in the broadcast payload*/
    }BroadcastMessage_t;
    
   /** The Network Clients registry
    * the network uses this placeholder to register all the nodes. 
    */
    HostClients_t DynamicHostClients[256];
    
    /**
     * The UID of this node
     */
    uint32_t ownUid;

    /**
     * placeholder to determine if this node is the host controller
     */
    bool isHostController; // if true, means this node is the host controller
    

    /** Set the UID of this node
     * the 4 byte UID is like the MAC address. It is used only before joining the network. 
     * @param uid the 4 byte unique identification number
     */
    void setUID(uint32_t uid);
    
    /** the UID of the network
     * the 4 byte UID of the network. this will filter any unnecessary broadcasting if multiple networks are in the same area
     * @param id 4-byte network unique identification number
     */
    void NetworkUID(uint32_t id);
    
    
    /**enable broadcast
     * if enabled, broadcast packets can be received on pipe 0
     * disable if broadcasting feature needs to be suspended once all adjacent pipes are accommodated  
     * @param sel 1 to enable, 0 to disable
     */
    void enableBroadcast(bool sel);
    
    /** process broadcast payload
     * once a broadcast packet is received, it needs to be processed by looking into its command. 
     * @param payload the 32 byte payload that has been received in the pipe 0 (broadcast dedicated pipe)
     * @return error status -1 for error, 0 for success
     */
    int processBroadcastPacket(Payload_t *payload);
    
    /**broadcast a payload
     * send a payload to the broadcast address set by other nodes
     * @param payload
     * @return error status -1 for error, 0 for success 
     */
    int broadcastPacket(Payload_t *payload);
    
    /**check if adjacent pipes is available
     * will return value between 0 and 4 corresponding to pipe 1 to 5
     * @return the pipe available to allow a new node to connect as its friend
     */
    int adjacentPipeAvailable();
    
    /**obtain node address from dhc
     * once a new node is accepted by a neighboring node as its friend, the friend node requests the dhc for a node address for the new node
     * @param message the structure containing the information such as the new nodes UID, and the broadcast command, etc. 
     * @return 2 byte node address assigned by the dhc
     */
    uint16_t ObtainAddressDhcAdjacent(BroadcastMessage_t *message);
    /** requrest to join the network
     * used by the new node to join the network
     * @return returns success or fail 
     */
    int requestNetworkJoin();
    
    int assignToAdjacent(adjacentNode_t *node);
    
    
    
private:

};

#endif /* NRF24L01PNETWORK_H */


