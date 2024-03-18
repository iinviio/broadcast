#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define PORT 8080

int id; /*id of this node*/
int seq_num = 0; /*seq_num is used by many functions*/

int* received_packets;
int received_packets_len = 0;


typedef struct Packet{

    int id;
    int seq_num;
    int data;

}Packet;

/*len is the size of neighbors*/
int validate_packet(Packet p, int* neighbors, int len){/*if the packet has been sent from a neighbor and it's not in the buffer then update buffer, seq_num and return 1, else return 0*/

    for(int i = 0; i < len; i++){

        /*check if the packet has been sent from a neighbor*/
        if(p.id == neighbors[i]){

            /*check whether the packet has already been received or not*/
            for(int j = received_packets_len - 1; j >= 0; j--){/*starts looking from the last received packet*/

                if(received_packets[j] == p.seq_num){/*if the packet has already been received, discard it*/

                    return -1;
                }
            }
            
            /*if the packet is new store it in the received_packet buffer and update seq_num*/
            received_packets = realloc(received_packets, ++received_packets_len);
            *(received_packets + received_packets_len - 1) = p.seq_num; /*add the seq_num of the last received packet*/
            seq_num++;

            return 0;
        }
    }

    return -1;
}

/*len is the length of neighbors, hence, the number of neighboring nodes*/
int _send(int data, int len, int sock, struct sockaddr_in* addr, socklen_t addrlen){/*returns -1 in case of error, 0 otherwise*/

    Packet p;
    p.id = id;
    p.seq_num = seq_num; /*assign seq_num*/
    p.data = data;

    printf("Sending: %d, seqnum : %d, id : %d\n", p.data, p.seq_num, p.id);

    if(sendto(sock, &p, sizeof(p), 0, (const struct sockaddr*) addr, addrlen) == -1){/*no need of flags, hence the 0 in the arguments*/
    
        return -1;
    }

    return 0;
}

/*buf is where the received value will be stored. I expect to receive an integer*/
int _receive(int sock, Packet* packet, struct sockaddr_in* addr, socklen_t* addrlen){/*returns -1 in case of error, 0 otherwise. Stores the received packet into "packet"*/

    /*no flags, hence 0*/
    if(recvfrom(sock, packet, sizeof(*packet), 0, (struct sockaddr*) addr, addrlen) == -1){

        return -1;
    }

    return 0;
}

int main(int argc, char** argv){

    const int optval = 1;
    int sock, seq_num = 0;
    
    struct sockaddr_in addr;
    socklen_t addrlen = (socklen_t) sizeof(addr);
    Packet packet = {0};
    

    /*input check*/
    if(argc < 3){

        perror("Not enough parameters");
        exit(EXIT_FAILURE);
    }

    id = atoi(argv[1]);/*assignment of id variable*/

    int neighbors_len = argc - 2;
    int neighbors [neighbors_len];

    for(int i = 0; i < neighbors_len; i++){

        neighbors[i] = atoi(argv[i + 2]);/*assignment of neighboring nodes*/
    }



    /*socket setup*/
    sock = socket(AF_INET, SOCK_DGRAM, 0);/*udp socket*/

    if(sock == -1){/*error check*/

        perror("Unable to create the socket ");
        exit(EXIT_FAILURE);
    }

    /*socket options + error check*/
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1){/*option managed at socket level (SOL_SOCKET). SO_BROADCAST option is set to true (optval, see man(7) socket)*/

        perror("Setsockopt error ");
        exit(EXIT_FAILURE);
    }

    /*set the address (addr)*/
    memset(&addr, 0, sizeof(addr));/*fills addr with 0*/
    addr.sin_family = AF_INET;/*allows communications over the network*/
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);/*uses broadcast address, converted from host byte order to network byte order*/
    addr.sin_port = htons(PORT);/*set 8080 port as default*/

    /*bind socket*/
    if(bind(sock, (const struct sockaddr*) &addr, (socklen_t) sizeof(addr)) == -1){

        perror("Unable to bind ");
        exit(EXIT_FAILURE);
    }


    /*the leader node start transmitting after 1 sec*/
    if(id == 0){
    
        sleep(1);/*1 sec. timeout before send the pakcet*/

        /*packet content*/
        srand(time(NULL)); /*rand seed*/
        int data = rand();/*generates a pseudo-ranodm number between 0 and RAND_MAX*/
        
        if(_send(data, argc - 2, sock, &addr, addrlen) == -1){

            perror("Unable to send the packet ");
            exit(EXIT_FAILURE);
        }
    }

    /*
        i'm assuming that there will be only one packet sent over the network
        i'm assuming that it'll be unlikely that a node will be required to both send and receive (so I am not implementing two separate threads to do so)
    */  

    if(_receive(sock, &packet, &addr, &addrlen)){

        perror("Error on recvfrom ");
        exit(EXIT_FAILURE);
    }

    printf("Received: %d, seqnum : %d, id : %d\n", packet.data, packet.seq_num, packet.id);

    if(validate_packet(packet, neighbors, neighbors_len) == -1){

        printf("Discarded: %d, seqnum : %d, id : %d\n", packet.data, packet.seq_num, packet.id);
        return 0;
    }

    if(_send(packet.data, argc - 2, sock, &addr, addrlen) == -1){

            perror("Unable to send the packet ");
            exit(EXIT_FAILURE);
    }

    return 0;
}