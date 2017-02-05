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
        uint8_t payloadData[32];
        Payload_t payload;
        payload.data = payloadData;
        payload.address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(AdjNode->nodeId)<<8) + (uint64_t)(0xC0+AdjNode->rxPipe);
        memcpy(payload.data, NetPayload, 7);
        memcpy(&payload.data[7], NetPayload->payload, NetPayload->length);
        payload.length = NetPayload->length + 7;
        TransmitPayload(&payload);     
}

void NRF24L01pNetwork::processNetworkPayload(Payload_t *payload){
    networkPayload_t *network_pld = (networkPayload_t*) payload->data;

    printf("DATA P%d, LENGTH %d: \r\n", payload->pipe, payload->length);
    int i;
    for(i=0;i<32;i++){
        if(i%8 == 0) printf("\r\n");
        printf("%x\t", payload->data[i]);  
    }
    printf("\r\n\r\n");
    
}