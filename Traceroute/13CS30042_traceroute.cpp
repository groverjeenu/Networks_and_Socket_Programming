// Assignment 5
// Implementing Traceroute using raw sockets

// Objective
// Utilize Raw Sockets this knowledge to implement traceroute which gives the similar result as above.

// Group Details
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: traceroute.cpp

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <iomanip>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/time.h>

#define MAX_PACKET_LEN 8192
#define MAX_DATA_LEN 8100
#define PACKET_LEN 60
#define TIMEVAL 1

using namespace std;

int ID = 1;

struct timeval tp;
long long int time_in_mill_1;
long long int time_in_mill_2;
long long int latency_1;
long long int latency_2;
long long int latency_3;


void getIPAddr(string hostName,sockaddr_in *saddr)
{
    unsigned int address;
    hostent *he;
    struct in_addr **addr_list;

    memset(saddr,0,sizeof(*saddr));

    address = inet_addr(&hostName[0]);

    if(address == INADDR_NONE)
    {
        he = gethostbyname(&hostName[0]);
        if(he == 0)
        {
            cout<<"Could Not Resolve Host "<<hostName<<endl;
            exit(1);
        }
        else
        {
            saddr->sin_family = AF_INET;
            addr_list = (struct in_addr **) he->h_addr_list;
     
            for(int i = 0; addr_list[i] != NULL; i++) 
            {
                //Return the first one;
                memcpy((void *)&(saddr->sin_addr), he->h_addr_list[i], he->h_length);
                //(saddr->sin_addr) = *addr_list[i];

                //cout<<"destIP: "<<inet_ntoa(*addr_list[i])<<endl;
                break;
            }
            //memset(&(saddr->sin_addr), (char *)hp->h_addr, hp->h_length);
        }
    }

    else
    {
        (saddr)->sin_family = AF_INET;
        (saddr->sin_addr).s_addr = address;
    }
}

int main(int argc,char *argv[])
{
    if(argc<2)
    {
        cout<<argv[0]<<" Destination"<<endl;
        exit(1);
    }

    int recvServFD,sendServFD,i=1,ttl = 1;
    sockaddr_in srcSaddr,destSaddr;
    struct iphdr *recvIP;
    struct icmphdr *recvICMP;
    char *output,*dest_IP;
    string destHostName(argv[1]);

    // Get Destination IP
    getIPAddr(destHostName,&destSaddr);

    char *destIP = inet_ntoa(destSaddr.sin_addr);

    destSaddr.sin_port = 10002;


    sendServFD = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    int send_PORT_NO = 10003;

    if(sendServFD < 0)printf("Server socket could not be connected\n");
    struct sockaddr_in saddr_a;
    saddr_a.sin_family = AF_INET;
    saddr_a.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr_a.sin_port = htons(send_PORT_NO);

    while(bind(sendServFD, (struct sockaddr *)&saddr_a,sizeof(saddr_a)) <0 )
    {
        printf("Bind Unsuccessful\n");
        cin>>send_PORT_NO;
        saddr_a.sin_port = htons(send_PORT_NO);
    }

    recvServFD = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(recvServFD<0)
    {
        perror("Socket() error");
        exit(-1);
    }

    fd_set active_sock_fd_set, read_fd_set;

    struct  timeval timeout;
    timeout.tv_sec = TIMEVAL;
    timeout.tv_usec = 0;

    cout<<"Traceroute to "<<destHostName<<"("<<destIP<<"), 30 hops max, 60 bytes packets"<<endl;

    FD_ZERO(&active_sock_fd_set);
    FD_SET(recvServFD, &active_sock_fd_set);

    while(true)
    {
        FD_ZERO(&active_sock_fd_set);
        FD_SET(recvServFD, &active_sock_fd_set);

        read_fd_set = active_sock_fd_set;

        timeout.tv_sec = TIMEVAL;
        timeout.tv_usec = 0;

        char in[60];

        strcpy(in,"Hello");

        gettimeofday(&tp,NULL);
        time_in_mill_1 = 1000000 * tp.tv_sec + tp.tv_usec;

        if(setsockopt(sendServFD,IPPROTO_IP,IP_TTL,(char *)&ttl,sizeof(ttl)) < 0)
        {
            perror("setsockopt() error");
            exit(-1);
        }

        if(sendto(sendServFD,in, strlen(in), 0, (struct sockaddr *)&destSaddr, sizeof(destSaddr)) < 0)
        {
            perror("sendto() error");
            exit(-1);
        }


        

        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) <= 0)
        {
            cout<<i<<" * * *"<<endl;
        }
    
        //cout<<"Successfully Sent"<<endl;

        else{
            if(FD_ISSET(recvServFD, &read_fd_set)){
                output = (char *)malloc(MAX_PACKET_LEN*sizeof(char));

                struct sockaddr_in saddr;
                int saddr_len = sizeof(saddr);
                int buflen = recvfrom(recvServFD,output,MAX_PACKET_LEN,0,(struct sockaddr *)(&saddr),(socklen_t *)&saddr_len);

                if(buflen<0)
                {
                    printf("error in reading recvfrom function\n");
                    return 0;
                }

                gettimeofday(&tp,NULL);
                time_in_mill_2 = 1000000 * tp.tv_sec + tp.tv_usec;

                latency_1 = time_in_mill_2 - time_in_mill_1;


                gettimeofday(&tp,NULL);
                time_in_mill_1 = 1000000 * tp.tv_sec + tp.tv_usec;

                if(sendto(sendServFD,in, strlen(in), 0, (struct sockaddr *)&destSaddr, sizeof(destSaddr)) < 0)
                {
                    perror("sendto() error");
                    exit(-1);
                }

                //cout<<"Successfully Sent"<<endl;

                output = (char *)malloc(MAX_PACKET_LEN*sizeof(char));

                saddr_len = sizeof(saddr);
                buflen = recvfrom(recvServFD,output,MAX_PACKET_LEN,0,(struct sockaddr *)(&saddr),(socklen_t *)&saddr_len);

                if(buflen<0)
                {
                    printf("error in reading recvfrom function\n");
                    return 0;
                }

                gettimeofday(&tp,NULL);
                time_in_mill_2 = 1000000 * tp.tv_sec + tp.tv_usec;

                latency_2 = time_in_mill_2 - time_in_mill_1;

                gettimeofday(&tp,NULL);
                time_in_mill_1 = 1000000 * tp.tv_sec + tp.tv_usec;

                if(sendto(sendServFD,in, strlen(in), 0, (struct sockaddr *)&destSaddr, sizeof(destSaddr)) < 0)
                {
                    perror("sendto() error");
                    exit(-1);
                }

                //cout<<"Successfully Sent"<<endl;

                output = (char *)malloc(MAX_PACKET_LEN*sizeof(char));

                saddr_len = sizeof(saddr);
                buflen = recvfrom(recvServFD,output,MAX_PACKET_LEN,0,(struct sockaddr *)(&saddr),(socklen_t *)&saddr_len);

                if(buflen<0)
                {
                    printf("error in reading recvfrom function\n");
                    return 0;
                }

                gettimeofday(&tp,NULL);
                time_in_mill_2 = 1000000 * tp.tv_sec + tp.tv_usec;

                latency_3 = time_in_mill_2 - time_in_mill_1;

                recvIP = (struct iphdr *)output;
                recvICMP = (struct icmphdr *)(output+sizeof(struct iphdr));

                /*if((unsigned int)(recvICMP->type) == 3)
                {
                    cout<<"Destination Unreachable"<<endl;
                    //exit(1);
                }*/

                dest_IP = (char *)malloc(100*sizeof(char));

                inet_ntop(AF_INET, &(recvIP->saddr), dest_IP, INET_ADDRSTRLEN);

                struct sockaddr_in sa;
                char host[1024];
                char service[20];

                sa.sin_family = AF_INET;
                sa.sin_addr.s_addr = recvIP->saddr;

                //struct hostent *he;
                //struct in_addr ipv4addr;

                //inet_pton(AF_INET, dest_IP, &ipv4addr);
                //he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);


                //getnameinfo((struct sockaddr *)&sa, sizeof saddr, host, sizeof host, service, sizeof service, 0);

                if(recvIP->saddr == destSaddr.sin_addr.s_addr)
                {
                    cout<<i<<"\t"<<argv[1]<<" ("<<dest_IP<<")\t"<<(1.0*(latency_1/1000.0))<<" ms\t\t"<<(1.0*(latency_2/1000.0))<<" ms\t\t"<<(1.0*(latency_3/1000.0))<<" ms"<<endl;
                    break;
                }

                else{
                    cout<<i<<"\t"<<dest_IP<<" ("<<dest_IP<<")\t\t"<<(1.0*(latency_1/1000.0))<<" ms\t\t"<<(1.0*(latency_2/1000.0))<<" ms\t\t"<<(1.0*(latency_3/1000.0))<<" ms"<<endl;
                }
            }



        }

        ttl++;
        i++;

        if(ttl > 30) break;
    }

    close(sendServFD);
    close(recvServFD);

    return 0;
}

