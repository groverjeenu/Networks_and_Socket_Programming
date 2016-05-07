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

class SMTP_Client{
	public:
        void sendMsg(){
            uint32_t dataLength = htonl(sendBuf.size()); // Ensure network byte order
            // when sending the data length
            //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
            send(connfd,sendBuf.c_str(),sendBuf.size(),MSG_CONFIRM); // Send the string
            //cout<<"Sent: "<<sendBuf<<endl;
        }

        string receiveMsg(){
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
            //cout << "\033[1;31mbold red text\033[0m\n";
            if(msg.substr(0,3) == "ERR" || msg.substr(0,3) == "550" || msg.substr(0,3) == "553") cout<< "\033[1;31m"<<msg<<"\033[0m\n";

            else cout<< "\033[1;32m"<<msg<<"\033[0m\n";
            return msg;
        }

        void connect(){
            string ready = receiveMsg();
            
            if(ready.substr(0,3) == "220"){
                // proceed
                // Send HELO
                sendBuf.clear();
                sendBuf = "HELO: "+hostName;
                sendMsg();
                string OK = receiveMsg();

                if(OK.substr(0,3) == "250") ;// fine
                else ; // error
            }
            else ;// error
        }

        void VRFY(){
        	string user;
        	cout<<"Enter Username: ";
        	cin>>user;
        	sendBuf.clear();
        	sendBuf = "VRFY "+user;
        	sendMsg();

        	string ack;
        	ack = receiveMsg();
        }

        void sendMail(){
            sendBuf.clear();
            string mailFrom,mailTo,subject,body,ack;

            // MAIL FROM
            cout<<"Mail From: ";
            cin>>mailFrom;
            sendBuf = "MAIL FROM: "+mailFrom;
            sendMsg();
            ack.clear();
            ack = receiveMsg();
            if(ack == "250 OK")
            {
                // continue
            }

            else ;// error

            // RCPT TO

            cout<<"Mail To: ";
            cin>>mailTo;
            sendBuf = "RCPT TO: "+mailTo;
            sendMsg();
            ack.clear();
        	ack = receiveMsg();

        	int reset = 0;

            while(1){
            	string rc_op;
            	cout<<"Choose an option: "<<endl;
            	cout<<"1. Add Receipent"<<endl;
            	cout<<"2. Send Data"<<endl;
            	cout<<"3. Reset (RSET)"<<endl;
            	cin>>rc_op;
            	if(rc_op == "1"){
		        	cout<<"CC: ";
		            cin>>mailTo;
		            sendBuf = "RCPT TO: "+mailTo;
		            sendMsg();
		            ack.clear();
	            	ack = receiveMsg();
	            }
	            else if(rc_op == "2"){
	            	break;
	            }
	            else if(rc_op == "3"){
	            	sendBuf = "RSET";
		            sendMsg();
		            ack.clear();
	            	ack = receiveMsg();
	            	reset = 1;
	            	break;	
	            }
	            else{
	            	cout<<"Invalid Option.....Try Again"<<endl;
	            }
        	}

            /*ack.clear();
            ack = receiveMsg();
            if(ack == "250 OK")
            {
                // continue
            }

            else ;// error*/

            if(reset == 0){
	            // DATA
	            sendBuf =  "DATA";
	            sendMsg();
	            ack.clear();
	            ack = receiveMsg();
	            if(ack == "354 start mail input")
	            {
	                // continue
	            }

	            else ;// error

	            // Start Mail Input
	            char c;
	            sendBuf.clear();
	            do
	            {
	                body.clear();
	                cin>>body;
	                sendBuf += body;
	                c = getchar();
	                sendBuf += c;
	            }while(body!=".");

	            sendMsg();
	            ack.clear();
	            ack = receiveMsg();
	            if(ack == "250 OK")
	            {
	                // continue
	            }
	            else ;// error
        	}

        	else{
        		return;
        	}
        }

        void terminate(){
            string ack;

            sendBuf.clear();
            sendBuf = "QUIT";
            sendMsg();

            ack = receiveMsg();
            if(ack == "221 service closed")
            {
                cout<<"Connection Closed Successfully"<<endl;
            }
            else{
                cout<<"There was error terminating connection....Try Again"<<endl;
            }
        }

        string sendBuf;
        string output;
        string IP;
        string hostName;
        int connfd;
        int loggedIn;
};

SMTP_Client garudaSMTPClient;

class POP_Client{
	public:
        void sendMsg(){
            uint32_t dataLength = htonl(sendBuf.size()); // Ensure network byte order
            // when sending the data length
            //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
            send(connfd,sendBuf.c_str(),sendBuf.size(),MSG_CONFIRM); // Send the string
            //cout<<"Sent: "<<sendBuf<<endl;
        }

        string receiveMsg(){
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
            if(msg.substr(0,3) == "ERR" || msg.substr(0,3) == "550" || msg.substr(0,3) == "553") cout<< "\033[1;31m"<<msg<<"\033[0m\n";

            else cout<< "\033[1;32m"<<msg<<"\033[0m\n";
            return msg;
        }

        void terminate()
        {
        	sendBuf.clear();
    		sendBuf = "QUIT";
    		sendMsg();
    		receiveMsg();
        }

        void receiveReady(string uid)
        {
            string ready;
            ready = receiveMsg();
            sendBuf.clear();
            sendBuf = "UID "+uid;
            sendMsg();
            receiveMsg();
        }

        void login()
        {
        	string ready,username,password,ack;

        	if(loggedIn == 0){

        		

	        	while(1)
	        	{
	        		cout<<"Enter Username: ";
	        		username.clear();
	        		cin>>username;

	        		sendBuf.clear();
	        		sendBuf = "USER "+username;
	        		sendMsg();

	        		ack.clear();
	        		ack = receiveMsg();
	        		if(ack == "OK")
	        		{
	        			break;
	        		}
	        		else if(ack == "ERR"){
	        			continue;
	        		}

	        	}


	        	while(1)
	        	{
	        		cout<<"Enter Password: ";
	        		password.clear();
	        		cin>>password;

	        		sendBuf.clear();
	        		sendBuf = "PASS "+password;
	        		sendMsg();

	        		ack.clear();
	        		ack = receiveMsg();
	        		if(ack == "OK")
	        		{
	        			break;
	        		}
	        		else if(ack == "ERR"){
	        			continue;
	        		}

	        	}

	        	cout<<"Successfully logged in"<<endl;
        	}
        	else{
                cout<<"You are logged in."<<endl;
            }
            loggedIn = 1;
            while(1){
                cout<<"Unread Mails:"<<endl;
                cout<<"1. Get STAT"<<endl;
                cout<<"2. Get LIST"<<endl;
                cout<<"3. Retreive a Mail (Please Check the LIST first)"<<endl;
                cout<<"Read Mails:"<<endl;
                cout<<"4. Get STAT"<<endl;
                cout<<"5. Get LIST"<<endl;
                cout<<"6. Retreive a Mail (Please Check the LIST first)"<<endl;
                cout<<"7. Go to Main Menu"<<endl;
                cout<<"8. Quit"<<endl;
                string tr_op;
                cin>>tr_op;

                if(tr_op == "1")
                {
                    string res;
                    sendBuf.clear();
                    sendBuf = "STAT";
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }
                else if(tr_op == "2")
                {
                    string res;
                    
                    sendBuf.clear();
                    sendBuf = "LIST";
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }
                else if(tr_op == "3")
                {
                    string msgid,res;
                    cout<<"Enter Message ID"<<endl;
                    cin>>msgid;

                    sendBuf.clear();
                    sendBuf = "RETR "+msgid;
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }

                else if(tr_op == "4")
                {
                    string res;
                    sendBuf.clear();
                    sendBuf = "STATR";
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }
                else if(tr_op == "5")
                {
                    string res;
                    
                    sendBuf.clear();
                    sendBuf = "LISTR";
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }
                else if(tr_op == "6")
                {
                    string msgid,res;
                    cout<<"Enter Message ID"<<endl;
                    cin>>msgid;

                    sendBuf.clear();
                    sendBuf = "RETRR "+msgid;
                    sendMsg();

                    res.clear();
                    res = receiveMsg();
                }

                else if(tr_op == "7")
                {
                    break;
                }
                else if(tr_op == "8")
                {
                    if(loggedIn == 1){
                        terminate();
                    }
                    if(garudaSMTPClient.loggedIn == 1){
                        garudaSMTPClient.terminate();
                    }
                    terminated = 1;
                    break;
                }

                else{
                    cout<<"Invalid Command...Try Again"<<endl;
                }

            }

        }

        string sendBuf;
        string output;
        string IP;
        string hostName;
        int connfd;
        int loggedIn;
        int terminated;

};

POP_Client garudaPOPClient;

int main(int argc, char * argv[]){
	int PORT_NO = atoi(argv[2]);
	int clientFD = socket(AF_INET,SOCK_STREAM,0),n;
    if(clientFD < 0)printf("Socket formation Failed\n");

    struct sockaddr_in saddr;
    printf("Tryint to Connect ...\n");

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT_NO);

    if(inet_pton(AF_INET,argv[1],&saddr.sin_addr) <= 0)printf("Addr conv Failed\n");

    if(connect(clientFD,(struct  sockaddr*)&saddr,sizeof(saddr)) < 0)printf("Connection Failed\n");
    //printf("\033[H\033[J");
    //printf("Connection established ...\n");


    int PORT_NO_p = atoi(argv[3]);
	int clientFD_p = socket(AF_INET,SOCK_STREAM,0);
    if(clientFD_p < 0)printf("Socket formation Failed\n");

    struct sockaddr_in saddr_p;
    printf("Tryint to Connect ...\n");

    saddr_p.sin_family = AF_INET;
    saddr_p.sin_port = htons(PORT_NO_p);

    if(inet_pton(AF_INET,argv[1],&saddr_p.sin_addr) <= 0)printf("Addr conv Failed\n");

    if(connect(clientFD_p,(struct  sockaddr*)&saddr_p,sizeof(saddr_p)) < 0)printf("Connection Failed\n");
    printf("\033[H\033[J");
    printf("Connection established to Garuda Server...\n");

    garudaSMTPClient.connfd = clientFD;

    
    garudaPOPClient.connfd = clientFD_p;
    garudaPOPClient.loggedIn = 0;
    garudaSMTPClient.loggedIn = 0;
    garudaPOPClient.terminated = 0;

    // Connection Successful with GARUDA SERVER

    string uid = argv[4];

    garudaSMTPClient.connect();
    garudaPOPClient.receiveReady(uid);
    garudaSMTPClient.loggedIn = 1;


    string op;
    cout<<"Do you want to (1) Send Emails or (2) Retreive Emails (3) Quit?"<<endl;
    cin>>op;

    int flag = 0;

    flag = 1;

    while(1){

        if(op == "1")
        {
        	string SMTP_op;
        	if(flag == 0)
        	{
        		garudaSMTPClient.connect();
        		garudaSMTPClient.loggedIn = 1;
        		flag = 1;
        	}

        	cout<<"Select an option:"<<endl;
        	cout<<"1. Send a Mail"<<endl;
        	cout<<"2. Check a User (VRFY)"<<endl;
        	cout<<"3. Go to Main Menu"<<endl;
        	cout<<"4. Quit"<<endl;

        	cin>>SMTP_op;
        	if(SMTP_op == "1")
        	{
        		garudaSMTPClient.sendMail();
        	}
        	else if(SMTP_op == "2")
        	{
        		garudaSMTPClient.VRFY();	
        	}
        	else if(SMTP_op == "3")
        	{
        		cout<<"Do you want to (1) Send Emails or (2) Retreive Emails (3) Quit?"<<endl;
    			cin>>op;
        	}
        	else if(SMTP_op == "4")
        	{
        		if(garudaSMTPClient.loggedIn == 1)
        		{
        			garudaSMTPClient.terminate();
        		}

        		garudaPOPClient.terminate();
        		
        		break;
        	}
        }

        else if(op == "2")
        {
        	garudaPOPClient.login();

        	if(garudaPOPClient.terminated == 0){
        		cout<<"Do you want to (1) Send Emails or (2) Retreive Emails? (3) Quit"<<endl;
    			cin>>op;
    		}
    		else break;
        }
        else if(op == "3"){
            if(garudaSMTPClient.loggedIn == 1)
            {
                garudaSMTPClient.terminate();
            }

            garudaPOPClient.terminate();
            
            break;
        }
        else{
            cout<<"Invalid Command...Try Again"<<endl;
        }
    }

	return 0;
}