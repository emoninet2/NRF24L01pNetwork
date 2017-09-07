/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NRF24L01pNetwork.cpp
 * Author: emon
 * 
 * Created on September 7, 2017, 9:53 AM
 */

#include "NRF24L01pNetwork.h"

NRF24L01pNetwork::NRF24L01pNetwork() {
}

NRF24L01pNetwork::NRF24L01pNetwork(const NRF24L01pNetwork& orig) {
}

NRF24L01pNetwork::~NRF24L01pNetwork() {
}

void NRF24L01pNetwork::initNetwork(uint16_t networkId, uint16_t nodeId){
    RadioConfig.DataReadyInterruptEnabled = 1;
    RadioConfig.DataSentInterruptEnabled = 1;
    RadioConfig.MaxRetryInterruptEnabled = 1;
    RadioConfig.Crc = NRF24L01p::CONFIG_CRC_16BIT;
    RadioConfig.AutoReTransmissionCount = 15;
    RadioConfig.AutoReTransmitDelayX250us = 15;
    RadioConfig.frequencyOffset = 2;
    RadioConfig.datarate = NRF24L01p::RF_SETUP_RF_DR_2MBPS;
    RadioConfig.RfPowerDb = NRF24L01p::RF_SETUP_RF_PWR_0DBM;
    RadioConfig.PllLock = 0;
    RadioConfig.ContWaveEnabled = 0;
    RadioConfig.FeatureDynamicPayloadEnabled = 1;
    RadioConfig.FeaturePayloadWithAckEnabled = 1;
    RadioConfig.FeatureDynamicPayloadWithNoAckEnabled = 1;

    int i;
    for(i=0;i<6;i++){
        RxPipeConfig[i].PipeEnabled = 1;
        RxPipeConfig[i].autoAckEnabled = 1;
        RxPipeConfig[i].dynamicPayloadEnabled = 1;
    }
    
    
    
    ownNodeId = nodeId;
    ownNetworkId = networkId;

    for(i=1;i<6;i++){
        RxPipeConfig[i].address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(ownNodeId)<<8) + (uint64_t)(NODE_PIPE_MASK+i);;
        set_RX_pipe_address((pipe_t)i,RxPipeConfig[i].address);
    }
    RoutingTableAddr = 0;
    
    
    Initialize();
}
void NRF24L01pNetwork::setAdjacentNode(pipe_t AssignedPipe, uint16_t adjNodeId, pipe_t AdjNodeRxPipe){
    if((AssignedPipe>= PIPE_P1)&&(AssignedPipe <= PIPE_P5)){
        AdjNode[AssignedPipe-1].nodeId = adjNodeId;
        AdjNode[AssignedPipe-1].rxPipe = AdjNodeRxPipe;
        AdjNode[AssignedPipe-1].status = AdjacentNodeStatus_Connected;
    }
}
void NRF24L01pNetwork::removeAdjacentNode(pipe_t AssignedPipe){
    if((AssignedPipe>= PIPE_P1)&&(AssignedPipe <= PIPE_P5)){
        AdjNode[AssignedPipe-1].status = AdjacentNodeStatus_Free;
    }
}
int NRF24L01pNetwork::sendToAdjacent(networkPayload_t *NetPayload, adjacentNode_t *AdjNode){
    
        Payload_t payload;     
        uint8_t payloadData[32];
        payload.UseAck = 1;
        payload.retransmitCount = 5;
        payload.address = ((uint64_t)ownNetworkId<<24) +( (uint64_t)(AdjNode->nodeId)<<8) + (uint64_t)(NODE_PIPE_MASK+AdjNode->rxPipe);
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
            printf("destination found on routing table\r\n");
            return sendToAdjacent(NetPayload, &RoutingTable[i].viaAdjNode);
        }
    }
    
    //forwarding to all adjacent nodes
    printf("estination unknown, forwarding to all adjacent\r\n");
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
    memcpy(NetPayload.payload, Payload->data, 25);
    //NetPayload.payload = &Payload->data[7];
    

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
            printf("destination found on routing table\r\n");
            return sendToAdjacent(&NetPayload, &RoutingTable[i].viaAdjNode);
        }
    }
    
    //forwarding to all adjacent nodes
    printf("destination unknown, forwarding to all adjacent\r\n");
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
        memcpy(AckPayload.payload, NetData, 25);
        sprintf((char*) AckPayload.payload, "ACK");

        sendToNetwork(&AckPayload);
        printf("ACK sent\r\n");
    }

}

void NRF24L01pNetwork::Network_init(){
    
}
void NRF24L01pNetwork::setUID(uint32_t uid){
    ownUid = uid;
}
void NRF24L01pNetwork::NetworkUID(uint32_t id){
    ownNetworkId = id;
}
void NRF24L01pNetwork::enableBroadcast(bool sel){
    set_RX_pipe_address((pipe_t) 0,NRF24L01P_NETWORK_BROADCAST_ADDR);
}
int NRF24L01pNetwork::processBroadcastPacket(Payload_t *payload){
    printf("\r\ngot broadcast packet\r\n");
    BroadcastMessage_t *message = (BroadcastMessage_t*)payload;
    
    //printf("\r\n");
    //int i;
    //for(i=0;i<32;i++){
        //printf("%x ", payload->data[i]);
    //}
    //printf("\r\n");
    
    printf("srcUID : %x\r\n", message->srcUID);
    printf("destUID : %x\r\n", message->destUID);
    printf("NetworkId : %x\r\n", message->NetworkID);
    printf("Command : %x\r\n", message->Cmd);
    
    BroadcastMessage_t respMesg ;
    respMesg.destUID = message->srcUID;
    respMesg.srcUID = ownUid;
    
    
    if(message->Cmd == GENERAL_CALL_REPLY){
        printf("\tgot general call\r\n");
        respMesg.Cmd = REPLY_GENERAL_CALL;
        broadcastPacket((Payload_t*)&respMesg);
    }
    else if(message->Cmd == PING_UID){
        if(message->destUID ==ownUid){
            printf("\tgot ping UID match\r\n");
            respMesg.Cmd = PONG_UID;
            broadcastPacket((Payload_t*)&respMesg);
        }
    }
    else if(message->Cmd == REQUEST_CONNECTION){
        if(message->destUID == ownUid){
            printf("requesting connection\r\n");
            ObtainAddressDhcAdjacent(message);

        }
    }  

    return 0;
}
int NRF24L01pNetwork::broadcastPacket(Payload_t *payload){
    payload->address = NRF24L01P_NETWORK_BROADCAST_ADDR;
    int ret = TransmitPayload(payload);
    return ret;   
}

int NRF24L01pNetwork::adjacentPipeAvailable(){
    int i;
    for(i=0;i<5;i++){
        if(AdjNode[i].status == AdjacentNodeStatus_Free) return i;
    }
    return -1;
}
uint16_t NRF24L01pNetwork::ObtainAddressDhcAdjacent(BroadcastMessage_t *message){
    //printf("obtain Node ID for UID : %x\r\n", message->srcUID);
    //uint16_t randNodeId = 0x45BA;
    
    BroadcastMessage_t respMesg;
    
    uint16_t newNodeId;
    int availableClientSlot = -1;
    int uidDuplicateInSlot = -1;
    int i;
    
    while(1){
        newNodeId = rand() % (65535 + 1 - 1) + 1;
        newNodeId += 4;
        bool NodeIdDuplicate = 0;
        for(i=0;i<265;i++){
            
            if( (availableClientSlot<0) && (DynamicHostClients[i].status == 0)){
                availableClientSlot = i;
            }
            
            if( (DynamicHostClients[i].uid == message->srcUID)){
                uidDuplicateInSlot = i;
            }
            
            if(newNodeId == DynamicHostClients[i].NodeId){
                NodeIdDuplicate = 1;
                break;
            }
        }

        if(uidDuplicateInSlot>=0){
            printf("ALREADY REGISTERED : %d\r\n", uidDuplicateInSlot);
            newNodeId = DynamicHostClients[uidDuplicateInSlot].NodeId;
        }

        
        if(NodeIdDuplicate == 0) {
            if(uidDuplicateInSlot == -1 ){
                printf("REGISTERING\r\n");
                printf("SLOT USED :  %d\r\n", availableClientSlot);
                printf("ASSIGNED NODE ID:  is %x\r\n", newNodeId);
                DynamicHostClients[availableClientSlot].NodeId = newNodeId;
                DynamicHostClients[availableClientSlot].uid = message->srcUID;
                DynamicHostClients[availableClientSlot].status |= 1;
            }
            
            break;
        }  
    }


    respMesg.Cmd = RESPOND_CONNECTION;
    respMesg.destUID = message->srcUID;
    respMesg.srcUID = ownUid;
    respMesg.NetworkID = ownNetworkId;
    memcpy((void*) respMesg.data,(void*) &newNodeId, sizeof(newNodeId));
    //respMesg.data[0] = (randNodeId);
    //respMesg.data[1] = (randNodeId>>8);


    broadcastPacket((Payload_t*)&respMesg);
    //printf("\tgonna say free pipe\r\n");
    
    
    return newNodeId;
}
int NRF24L01pNetwork::requestNetworkJoin(){
    printf("broadcasting to find nodes on network\r\n");
    BroadcastMessage_t message;
    BroadcastMessage_t message2;
    uint16_t newNodeId ;

    while(!((message.Cmd == REPLY_GENERAL_CALL) && (message.destUID == ownUid))){//loop until a general call reply is received
        message.destUID = 0;
        message.srcUID = ownUid;
        message.NetworkID = ownNetworkId;
        message.Cmd = GENERAL_CALL_REPLY;
        message.len = 32;
        
        broadcastPacket((Payload_t*)&message);
        port_DelayMs(1000);
    }
    
    printf("found node on network, node UID : %x\r\n", message.srcUID);
    
    printf("sending request to join network\r\n");
    
    while(!((message2.Cmd == RESPOND_CONNECTION) && (message2.destUID == ownUid ))){//loop until a general call reply is received
        //printf("requesting node ID\r\n");
        message2.destUID = message.srcUID;
        message2.srcUID = ownUid;
        message2.NetworkID = ownNetworkId;
        message2.Cmd = REQUEST_CONNECTION;
        message2.len = 32;
        
        broadcastPacket((Payload_t*)&message2);
        port_DelayMs(1000);
    }

    memcpy((void*) &newNodeId, (void*) message2.data, sizeof(newNodeId));
    printf("joined network successfully\r\nAssigned node ID is  : %x\r\n\r\n", newNodeId);
    
    ownNodeId = newNodeId;
    
    
    port_DelayMs(5000);
    return 0;    
}
int NRF24L01pNetwork::assignToAdjacent(adjacentNode_t *node){
    
}
