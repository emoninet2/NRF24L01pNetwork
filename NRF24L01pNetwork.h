/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NRF24L01pNetwork.h
 * Author: emon1
 *
 * Created on February 5, 2017, 11:35 PM
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
#include "NRF24L01pNetworkConfig.h"

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
        uint8_t *payload;
    }networkPayload_t;
    
    typedef struct{
        uint16_t nodeId;
        pipe_t rxPipe;
        uint8_t status;
    }adjacentNode_t;
    
    typedef struct{
        uint16_t destNodeId;
        adjacentNode_t viaAdjNode;
    }routingNode_t;
    
    uint16_t ownNodeId;
    uint16_t ownNetworkId;
    adjacentNode_t  AdjNode[5];
    routingNode_t     RoutingTable[20];
    unsigned int RoutingTableAddr;
    
    void initNetwork(uint16_t networkId, uint16_t nodeId);
    void setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe);
    void sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode);
    void processNetworkPayload(Payload_t *payload);
    void sendToNetwork(networkPayload_t *NetPayload);
    void forwardPacket(Payload_t *payload);
    void routingTableUpdate(Payload_t *payload);
    void sendAcknowledgement(Payload_t *payload);
private:

};

#endif /* NRF24L01PNETWORK_H */

