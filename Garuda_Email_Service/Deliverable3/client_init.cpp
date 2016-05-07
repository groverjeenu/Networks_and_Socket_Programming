#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <iomanip>
#include <sys/time.h>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

#define MESSAGE_LENGTH 50000

int bytes = MESSAGE_LENGTH;


void sendMsg(int connfd,string sendBuf){
    uint32_t dataLength = htonl(sendBuf.size()); // Ensure network byte order
    send(connfd,sendBuf.c_str(),sendBuf.size(),MSG_CONFIRM); // Send the string
    //cout<<"Sent: "<<sendBuf<<endl;
}

string receiveMsg(int connfd){
    string output;
    output.clear();
    output.resize(bytes);

    int bytes_received = read(connfd, &output[0], bytes-1);
    if (bytes_received<0)
    {
        std::cerr << "Failed to read data from socket.\n";
        return NULL;
    }
    output.resize(bytes_received);
    char* cstr = new char[output.length() +1];
    strcpy(cstr, output.c_str());
    string msg(cstr);
    //cout<<"Received: "<<msg<<endl;
    return msg;
}

int main(int argc, char * argv[])
{
    string IP,op,name;

    string SMTP_PORT,POP_PORT;

    int PORT_NO = atoi(argv[2]);
    int clientFD = socket(AF_INET,SOCK_STREAM,0),n;
    if(clientFD < 0)printf("Socket formation Failed\n");

    struct sockaddr_in saddr;
    printf("Trying to Connect ...\n");

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT_NO);

    if(inet_pton(AF_INET,argv[1],&saddr.sin_addr) <= 0)printf("Addr conv Failed\n");

    if(connect(clientFD,(struct  sockaddr*)&saddr,sizeof(saddr)) < 0)printf("Connection Failed\n");
    printf("\033[H\033[J");
    printf("Getting User Information\n");

    string uid;

    while(1)
    {
        string ack;
        cout<<"Enter your name: ";
        cin>>name;
        sendMsg(clientFD,name);

        ack = receiveMsg(clientFD);

        cout<<ack<<endl;

        if(ack.substr(0,2) == "OK")
        {
            uid = ack.substr(3);
            break;
        }
        else{
            continue;
        }
    }


    vector<string> connections;
    string newConnection;

    while(1)
    {
        cout<<"Please choose an option:"<<endl;
        cout<<"1. Connect to a new Server"<<endl;
        cout<<"2. Show All Connections"<<endl;
        cout<<"3. Exit"<<endl;

        cin>>op;

        if(op == "1")
        {
            cout<<"Enter Server's IP: ";
            cin>>IP;

            cout<<"Enter SMTP Port: ";
            cin>>SMTP_PORT;

            cout<<"Enter POP Port: ";
            cin>>POP_PORT;
            newConnection = IP+" "+SMTP_PORT+" "+POP_PORT;
            connections.push_back(newConnection);

            int pid = fork();

            if(pid == 0){
                execlp("xterm" ,"xterm" ,"-hold","-e","./client",IP.c_str(),SMTP_PORT.c_str(),POP_PORT.c_str(),uid.c_str(),(const char *)NULL);
                exit(1);
            }
        }

        else if(op == "2")
        {
            for(int i=0;i<connections.size();++i)
            {
                cout<<"-->> "<<connections[i]<<endl;
            }
        }

        else if(op == "3")
        {
            break;
        }

        else{
            cout<<"Invalid Option...Try Again"<<endl;
        }
    }
    return 0;
}