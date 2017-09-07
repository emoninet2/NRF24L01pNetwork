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



class NRF24L01pNetwork : public NRF24L01p{
public:
    NRF24L01pNetwork();
    NRF24L01pNetwork(const NRF24L01pNetwork& orig);
    virtual ~NRF24L01pNetwork();
    
    
    typedef struct {
        uint16_t srcNodeId;
        uint16_t destNodeId;
        uint8_t pid;
        uint8_t packetInfo;
        uint8_t length;
    }networkPayloadHeader_t;
    
    
    typedef struct {
        networkPayloadHeader_t Header;
        uint16_t srcNodeId; //TBR
        uint16_t destNodeId;//TBR
        uint8_t pid;//TBR
        uint8_t packetInfo;//TBR
        uint8_t length;//TBR
        uint8_t payload[25];
    }networkPayload_t;
    
    typedef enum{  
        AdjacentNodeStatus_Free = 0,
        AdjacentNodeStatus_Connected = 1,    
        AdjacentNodeStatus_Unresponsive = -1        
    }adjacentNodeStatus_t;
    
    
    
    typedef struct{
        uint16_t nodeId;
        pipe_t rxPipe;
        adjacentNodeStatus_t status;
    }adjacentNode_t;
    
    typedef struct{
        uint16_t destNodeId;
        adjacentNode_t viaAdjNode;
    }routingNode_t;
    
    uint16_t ownNodeId;
    uint16_t ownNetworkId;
    adjacentNode_t  AdjNode[5];
    routingNode_t     RoutingTable[ROUTING_TABLE_SIZE];
    unsigned int RoutingTableAddr;
    
    void initNetwork(uint16_t networkId, uint16_t nodeId);
    void setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe);
    void removeAdjacentNode(pipe_t AdjNodeRxPipe);
    int sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode);
    void processNetworkPayload(Payload_t *payload);
    int sendToNetwork(networkPayload_t *NetPayload);
    int forwardPacket(Payload_t *payload);
    void routingTableUpdate(Payload_t *payload);
    void sendAcknowledgement(Payload_t *payload);

    //////////////////////////////////////////////////////////////////////////////////////////////
    
    typedef enum{
        GENERAL_BROADCAST = 0xC0,
        GENERAL_CALL_REPLY = 0xC1,
        REPLY_GENERAL_CALL = 0xC2,
        PING_UID = 0xC3,
        PONG_UID = 0xC4,
        REQUEST_CONNECTION = 0xC5,
        RESPOND_CONNECTION = 0xC6,          
    }BroadcastCommand_t;
    
    
    typedef struct{
        uint16_t NodeId;
        uint16_t friendNodeId;
        uint32_t uid;
        uint8_t status;
    }HostClients_t;
    
    
    typedef struct{
        uint32_t srcUID;
        uint32_t destUID;
        uint16_t NetworkID;
        BroadcastCommand_t Cmd; 
        uint8_t len;
        uint8_t data[25];
    }BroadcastMessage_t;
    
    HostClients_t DynamicHostClients[256];
    uint32_t ownUid;

    bool isHostController; // if true, means the node is the host controller
    
    
    void Network_init();
    void setUID(uint32_t uid);
    void NetworkUID(uint32_t id);
    void enableBroadcast(bool sel);
    int processBroadcastPacket(Payload_t *payload);
    int broadcastPacket(Payload_t *payload);
    
    int adjacentPipeAvailable();
    uint16_t ObtainAddressDhcAdjacent(BroadcastMessage_t *message);
    int requestNetworkJoin();
    int assignToAdjacent(adjacentNode_t *node);
    
    
    
private:

};

#endif /* NRF24L01PNETWORK_H */


