/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NRF24L01pNetwork.cpp
 * Author: emon1
 * 
 * Created on February 5, 2017, 11:35 PM
 */

#include "NRF24L01pNetwork.h"

NRF24L01pNetwork::NRF24L01pNetwork() {
}

NRF24L01pNetwork::NRF24L01pNetwork(const NRF24L01pNetwork& orig) {
}

NRF24L01pNetwork::~NRF24L01pNetwork() {
}

void NRF24L01pNetwork::initNetwork(uint16_t networkId, uint16_t nodeId){
    ownNodeId = nodeId;
    ownNetworkId = networkId;
    
    int i;
    for(i=1;i<6;i++){
        RxPipeConfig[i].address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(ownNodeId)<<8) + (uint64_t)(0xC0+i);;
        set_RX_pipe_address((pipe_t)i,RxPipeConfig[i].address);
    }
}
void NRF24L01pNetwork::setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe){
    if((AssignedPipe>= PIPE_P1)&&(AssignedPipe <= PIPE_P5)){
        AdjNode[AssignedPipe-1].nodeId = adjNodeId;
        AdjNode[AssignedPipe-1].rxPipe = AdjNodeRxPipe;
        AdjNode[AssignedPipe-1].status = 1;
    }
}

void NRF24L01pNetwork::sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode){
        Payload_t payload;
        payload.address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(AdjNode->nodeId)<<8) + (uint64_t)(0xC0+AdjNode->rxPipe);
        printf("own network ID : %x\r\n", ownNetworkId);
        printf("sending to %llx\r\n", payload.address);
        memcpy(payload.data, NetPayload, 32);
        payload.length = 32;
        TransmitPayload(&payload);     
}
