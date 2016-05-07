#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <vector>
#include <map>
#include <iomanip>
#include <set>
#include <sqlite3.h>


using namespace std;

#define NO_OF_CLIENTS 20
#define MESSAGE_LENGTH 50000
#define PACKET_SIZE 8

int bytes = MESSAGE_LENGTH;

void sendMsg(int connfd,string sendBuf){
    uint32_t dataLength = htonl(sendBuf.size()); // Ensure network byte order
    // when sending the data length
    //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
    send(connfd,sendBuf.c_str(),sendBuf.size(),MSG_CONFIRM); // Send the string
    cout<<"Sent: "<<sendBuf<<endl;
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
    cout<<"Received: "<<msg<<endl;
    return msg;
}



static int callback(void *data, int argc, char **argv, char **azColName){
   int i;

   stringstream ss;

   string *uid = (string *)data;

   ss<<"OK "<<argv[0];
    *uid = ss.str();
   return 0;
}

void* check_user(void * args)
{
    int clientFD = *((int*)args);

    while(1)
    {
    	sqlite3 *db;
	    char *zErrMsg = 0;
	    int rc;
	    string sql;
	    string data;

	    rc = sqlite3_open("User.db", &db);

    	string name;
    	name = receiveMsg(clientFD);

    	sql = "SELECT * FROM User WHERE Name = '"+name+"';";
    	data = "ERR";
    	rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

        if(rc != SQLITE_OK)
        {
            cout<<"Error Querying Database"<<endl;
        }

        if(data.substr(0,2) == "OK")
        {
        	sendMsg(clientFD,data);
        	break;
        }
        else{
        	sendMsg(clientFD,data);
        }
    }
}


int main(int argc, char * argv[])
{
    int liveThreads;
	printf("\033[H\033[J");
    int servfd = socket(AF_INET,SOCK_STREAM,0);
    int PORT_NO = atoi(argv[1]);
    printf("%d\n",servfd);
    int pid;

    char s[3000];
    //system ("fuser -k 5000/tcp");

    // Server For SMTP    
    if(servfd < 0)printf("Server socket could not be connected\n");
    char  msg[MESSAGE_LENGTH];
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(PORT_NO);

    while(bind(servfd, (struct sockaddr *)&saddr,sizeof(saddr)) <0 )
    {
        printf("Bind Unsuccessful\n");
        cin>>PORT_NO;
        saddr.sin_port = htons(PORT_NO);
    }
    if(listen(servfd,NO_OF_CLIENTS) <0 )printf("Listen unsuccessful\n");


    
    
    while(1)
    {
        int connfd = accept(servfd, (struct sockaddr*)NULL, NULL);
        if(connfd >= 0)
        {
            liveThreads++;

            pthread_t td;

            if(connfd<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            cout<<"New Connection Started\tconnFD = "<<connfd<<endl;

            try
            {
                pthread_create(&td,NULL,&check_user,&connfd);

            }
            catch(const std::exception& e)
            {
                cout<<e.what()<<endl;
            }
            //cout<<"Thread is terminated"<<endl;

        }
    }
    return 0;
}