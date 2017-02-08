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
    RoutingTableAddr = 0;
}
void NRF24L01pNetwork::setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe){
    if((AssignedPipe>= PIPE_P1)&&(AssignedPipe <= PIPE_P5)){
        AdjNode[AssignedPipe-1].nodeId = adjNodeId;
        AdjNode[AssignedPipe-1].rxPipe = AdjNodeRxPipe;
        AdjNode[AssignedPipe-1].status = 1;
    }
}

int NRF24L01pNetwork::sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode){
        uint8_t payloadData[32];
        Payload_t payload;
        payload.UseAck = 1;
        payload.retransmitCount = 5;
        payload.data = payloadData;
        payload.address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(AdjNode->nodeId)<<8) + (uint64_t)(0xC0+AdjNode->rxPipe);
        memcpy(payload.data, NetPayload, 7);
        memcpy(&payload.data[7], NetPayload->payload, NetPayload->length);
        payload.length = NetPayload->length + 7;
        return TransmitPayload(&payload);   

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
    printf("source NodeID : %x\r\n", network_pld->srcNodeId);
    printf("destination NodeID : %x\r\n", network_pld->destNodeId);
    printf("PID : %x\r\n", network_pld->pid);
    printf("Packet Info : %x\r\n", network_pld->packetInfo);
    printf("Length : %x\r\n", network_pld->length);
    
    printf("PID : %x\r\n", network_pld->pid);
    routingTableUpdate(payload);
    
    if(network_pld->destNodeId == ownNodeId){
        printf("packet destination matched own ID\r\n");
        sendAcknowledgement(payload);
    }
    else{
        printf("forwarding packet\r\n");
        forwardPacket(payload);
    }
}
int NRF24L01pNetwork::sendToNetwork(networkPayload_t *NetPayload){
    
    //checking in Destination node is adjacent
    int i;
    for(i=0;i<5;i++){
        if(NetPayload->destNodeId == AdjNode[i].nodeId){
            printf("destination is adjacent node : %x\r\n",AdjNode[i].nodeId);
            return sendToAdjacent(NetPayload, &AdjNode[i]);
        }
    }
    
    //checking if destination node is on Routing Table
    for(i=0;i<20;i++){
        if((NetPayload->destNodeId == RoutingTable[i].destNodeId)){
            printf("on routing table\r\n");
            return sendToAdjacent(NetPayload, &RoutingTable[i].viaAdjNode);
        }
    }
    
    //forwarding to all adjacent nodes
    printf("forwarding to all adjacent\r\n");
    for(i=0;i<5;i++){
        if(AdjNode[i].status != 0){
            return sendToAdjacent(NetPayload, &AdjNode[i]);
        }
    }
}

int NRF24L01pNetwork::forwardPacket(Payload_t *Payload){
    //networkPayload_t *NetPayload = (networkPayload_t*) Payload->data;
    //NetPayload->payload = &Payload->data[7];
    
    networkPayload_t NetPayload;
    memcpy(&NetPayload, Payload->data, 7);
    NetPayload.payload = &Payload->data[7];
    

    adjacentNode_t viaNode;
    viaNode.nodeId = AdjNode[Payload->pipe - 1].nodeId;
    viaNode.rxPipe = AdjNode[Payload->pipe - 1].rxPipe;
    
    
    //checking in Destination node is adjacent
    int i;
    for(i=0;i<5;i++){
        if((NetPayload.destNodeId == AdjNode[i].nodeId)&&(NetPayload.destNodeId != viaNode.nodeId)  ){
            printf("destination is adjacent node : %x\r\n",AdjNode[i].nodeId);
            return sendToAdjacent(&NetPayload, &AdjNode[i]);
        }
    }
    
    //checking if destination node is on Routing Table
    for(i=0;i<20;i++){
        if((NetPayload.destNodeId == RoutingTable[i].destNodeId)){
            printf("on routing table\r\n");
            return sendToAdjacent(&NetPayload, &RoutingTable[i].viaAdjNode);
        }
    }
    
    //forwarding to all adjacent nodes
    printf("forwarding to all adjacent\r\n");
    for(i=0;i<5;i++){
        if((AdjNode[i].status != 0)&&(AdjNode[i].nodeId != viaNode.nodeId)){
            return sendToAdjacent(&NetPayload, &AdjNode[i]);
        }
    }
    
    
    
    //printf("forwarding data is : %s to node : %x\r\n", NetPayload.payload, viaNode.nodeId);
    //sendToAdjacent(&NetPayload, &viaNode);

}


void NRF24L01pNetwork::routingTableUpdate(Payload_t *payload){
    //printf("\r\tROUTING TABLE HANDLER\r\n");
    networkPayload_t *NetPayload = (networkPayload_t*) payload->data;
    int i;
    
    for(i=0;i<5;i++){
        if((NetPayload->srcNodeId == AdjNode[i].nodeId)  ){
            //printf("already Adjacent. Not storing\r\n");
            return;
        }
    }

    for(i=0;i<20;i++){
        if((NetPayload->srcNodeId == RoutingTable[i].destNodeId)  ){
            //printf("already on routing table. Not storing\r\n");
            return;
        }
    }
    //printf("storing to routing table\r\n");
    RoutingTable[RoutingTableAddr].destNodeId = NetPayload->srcNodeId;
    RoutingTable[RoutingTableAddr].viaAdjNode.nodeId = AdjNode[payload->pipe-1].nodeId;
    RoutingTable[RoutingTableAddr].viaAdjNode.rxPipe = AdjNode[payload->pipe-1].rxPipe;
    RoutingTableAddr++;
    if(RoutingTableAddr>=20) RoutingTableAddr = 0;
    
    //printf("New Routing table\r\n");
    for(i=0;i<20;i++){
        //printf("\t\t%x --> [%x:%d]\r\n",RoutingTable[i].NodeId,RoutingTable[i].FwrdAdjNode.NodeId,RoutingTable[i].FwrdAdjNode.RxPipe  );
    }
    
}


void NRF24L01pNetwork::sendAcknowledgement(Payload_t *payload){
    
    networkPayload_t *NetPayload = (networkPayload_t*) payload->data;
    
    
    uint8_t NetData[25];
    if(NetPayload->packetInfo&(1<<0)){
        //printf("\r\tSENDING ACKNOWLEDGEMENT\r\n");
        networkPayload_t AckPayload;
        AckPayload.destNodeId = NetPayload->srcNodeId;
        AckPayload.srcNodeId = ownNodeId;
        AckPayload.pid = NetPayload->pid;
        AckPayload.packetInfo = (NetPayload->packetInfo)&0b11111110;
        AckPayload.payload = NetData;
        sprintf((char*) AckPayload.payload, "ACK");

        sendToNetwork(&AckPayload);
        printf("ACK sent\r\n");
    }

}