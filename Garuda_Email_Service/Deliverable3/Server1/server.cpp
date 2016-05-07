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
string serverName = "abc.com";
int serverID = 1;

map<string,string> ServerIP;
map<string,int> ServerPort;

typedef struct ClientInfo
{
    string hostName;
    string mailFrom;
    string mailTo;
    string body;
    int state;  // (0--not Received HELO , 1--received HELO , 2--received MAIL, 3--received RCPT, 4--received DATA)
    int route; // 0 -- no , 1 -- Yes
}ClientInfo;

static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   //cout<<"in callback"<<endl;
   int *rows = (int *)data; 

   if(argc>0)
   {
        rows[0] = atoi(argv[0]);  // UID
        rows[1] = atoi(argv[1]);  // SID
   }
   return 0;
}

static int callback_stat(void *data, int argc, char **argv, char **azColName){
   int i;

   int *rows = (int *)data; 

   if(argc>0)
   {
        rows[0]++;  // No. Of mails
        rows[1] += atoi(argv[8]);  // size
   }
   return 0;
}


static int callback_list(void *data, int argc, char **argv, char **azColName){
   int i;
   stringstream ss;

   pair<int,string> *rows = (pair<int,string> *)data; 

   if(argc>0)
   {
        ((*rows).first)++;

        ss<<argv[0]<<" "<<argv[8]<<"B";
        ((*rows).second) += ss.str();
        ((*rows).second) += "\n";
        //rows[0]++;  // No. Of mails
        //rows[1] += atoi(argv[8]);  // size
   }
   return 0;
}


static int callback_retr(void *data, int argc, char **argv, char **azColName){
   int i;

   stringstream ss;
   string *rows = (string *)data;

   if(argc>0)
   {
        ss<<"OK "<<argv[8]<<"B\n"<<"From: "<<argv[1]<<"\nBody: "<<argv[7];
        *rows = ss.str();
   }
   return 0;
}


static int callback_vrfy(void *data, int argc, char **argv, char **azColName){
   int i;

   
   pair<int,int> *rows = (pair<int,int> *)data;

   if(argc>0)
   {
        ((*rows).first)++;
        ((*rows).second) = atoi(argv[0]);
   }
   return 0;
}

static int callback_final(void *data, int argc, char **argv, char **azColName){
   int i;

   
   pair<int,string> *rows = (pair<int,string> *)data;

   if(argc>0)
   {
        ((*rows).first) = atoi(argv[1]);
        ((*rows).second) = argv[2];
   }
   return 0;
}


static int callback_name(void *data, int argc, char **argv, char **azColName){
   int i;

   
   string *rows = (string *)data;

   if(argc>0)
   {
        *rows = argv[1]; 
   }
   return 0;
}

static int callback_rcpt(void *data, int argc, char **argv, char **azColName){
   int i;

   
   int *rows = (int *)data;

   if(argc>0)
   {
        *rows =  atoi(argv[1]);
   }
   return 0;
}

static int callback_insert(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}


static int callback_user(void *data, int argc, char **argv, char **azColName){
   int i;

   int *exists = (int *)data;

    *exists = 1;
   /*for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }*/
   //printf("\n");
   return 0;
}


static int callback_pass(void *data, int argc, char **argv, char **azColName){
   int i;

   int *exists = (int *)data;

    *exists = 1;
   /*for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }*/
   //printf("\n");
   return 0;
}




class SMTP_Server{
    public:
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

        void send220(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "220 service ready";
            sendMsg(clientFD,sendBuf);
        }

        void send221(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "221 service closed";
            sendMsg(clientFD,sendBuf);   
        }

        void send250(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "250 OK";
            sendMsg(clientFD,sendBuf);
        }

        void send354(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "354 start mail input";
            sendMsg(clientFD,sendBuf);
        }        

       // map<int,ClientInfo> clients;    // Map of Client FDs => Client info

        void handleVRFY(int clientFD,string user)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            pair<int,int> data_vrfy;    // 0 -- UID , 1 -- SID            
            int data_mail[2];    // 0 -- no. of mails , 1 -- total size

            rc = sqlite3_open("Garuda.db", &db);

            //cout<<username<<endl;

            data_vrfy.first = 0;

            sql = "SELECT * FROM User WHERE Name LIKE '%"+user+"%';";

            //cout<<sql<<endl;

            rc = sqlite3_exec(db, &sql[0], callback_vrfy, &data_vrfy, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database13"<<endl;
            }

            string reply;

            if(data_vrfy.first == 0)
            {
                reply = "550 String does not match anything.";
                sendMsg(clientFD,reply);
                return;
            }

            else if(data_vrfy.first > 1)
            {
                reply = "553 User ambiguous.";
                sendMsg(clientFD,reply);
                return;
            }

            int user_id = data_vrfy.second;

            stringstream sqlss;
            sqlss<<"SELECT * FROM User WHERE UID="<<user_id<<";";

            sql = sqlss.str();
            string data_name;

            rc = sqlite3_exec(db, &sql[0], callback_name, &data_name, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database14"<<endl;
            }

            string name = data_name;

            sqlss.str(std::string());

            sqlss<<"SELECT * FROM IsUserOf WHERE UID = "<<user_id<<";";
            sql.clear();
            sql = sqlss.str();

            pair<int,string> data_final;

            data_final.first = -1;

            rc = sqlite3_exec(db, &sql[0], callback_final, &data_final, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database15"<<endl;
            }

            if(data_final.first == -1)
            {
                reply = "550 String does not match anything.";
                sendMsg(clientFD,reply);
                return;
            }

            else if(data_final.first == serverID)
            {
                reply = "250 "+name+" <"+data_final.second+">";
                sendMsg(clientFD,reply);
                return;
            }

            else{
                reply = "551 User not local;";
                sendMsg(clientFD,reply);
                return;
            }
        }
};


class POP_Server{
    public:
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

        void sendReady(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "OK POP3 Server Ready";
            sendMsg(clientFD,sendBuf);
        }

        void sendOK(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "OK";
            sendMsg(clientFD,sendBuf);
        }

        void sendERR(int clientFD)
        {
            string sendBuf;
            sendBuf.clear();
            sendBuf = "ERR";
            sendMsg(clientFD,sendBuf);
        }

        int authorizeUser(string username,string uid)
        {

            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data = 0;    // 0 -- UID , 1 -- SID

            rc = sqlite3_open("Garuda.db", &db);

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"' AND UID = "+uid+";";

            rc = sqlite3_exec(db, &sql[0], callback_user, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database4"<<endl;
            }

            sqlite3_close(db);

            return data;

        }

        int authorizePass(string username,string password)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data = 0;    // 0 -- UID , 1 -- SID

            rc = sqlite3_open("Garuda.db", &db);


            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"' AND password = '"+password+"'";

            rc = sqlite3_exec(db, &sql[0], callback_pass, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database5"<<endl;
            }

            sqlite3_close(db);

            return data;
        }

        string STAT_Read(string username)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            int data_mail[2];    // 0 -- no. of mails , 1 -- total size

            rc = sqlite3_open("Garuda.db", &db);

            //cout<<username<<endl;

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"';";

            //cout<<sql<<endl;

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database6"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 1 ORDER BY EID DESC;";
            sql.clear();
            sql = sqlss.str();

            //cout<<sql<<endl;

            data_mail[0] = 0;
            data_mail[1] = 0;

            rc = sqlite3_exec(db, &sql[0], callback_stat, &data_mail, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database7"<<endl;
            }

            sqlite3_close(db);

            string result;
            stringstream resultss;

            resultss<<"OK "<<data_mail[0]<<" "<<data_mail[1]<<"B";
            result = resultss.str();
            return result;
        }

        string LIST_Read(string username)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            pair<int,string> data_list;    // 0 -- no. of mails , 1 -- total size

            rc = sqlite3_open("Garuda.db", &db);

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"'";

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database8"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 1 ORDER BY EID DESC;";
            sql.clear();
            sql = sqlss.str();


            data_list.first = 0;
            (data_list.second).clear();
            rc = sqlite3_exec(db, &sql[0], callback_list, &data_list, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database9"<<endl;
            }

            sqlite3_close(db);

            string result;
            stringstream resultss;

            resultss<<"OK "<<data_list.first<<" messages\n"<<data_list.second;
            result = resultss.str();
            return result;
        }

        string RETR_Read(string username,string id)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            string data_retr;

            rc = sqlite3_open("Garuda.db", &db);

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"'";

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database10"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 1 AND EID = "<<id<<";";
            sql.clear();
            sql = sqlss.str();


            data_retr.clear();
            rc = sqlite3_exec(db, &sql[0], callback_retr, &data_retr, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database11"<<endl;
            }

            sqlite3_close(db);

            if(data_retr.length() == 0)
            {
                string result = "ERR no such message";
                return result;
            }
            else return data_retr;   
        }


        string STAT(string username)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            int data_mail[2];    // 0 -- no. of mails , 1 -- total size

            rc = sqlite3_open("Garuda.db", &db);

            //cout<<username<<endl;

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"';";

            //cout<<sql<<endl;

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database6"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 0 ORDER BY EID DESC;";
            sql.clear();
            sql = sqlss.str();

            //cout<<sql<<endl;

            data_mail[0] = 0;
            data_mail[1] = 0;

            rc = sqlite3_exec(db, &sql[0], callback_stat, &data_mail, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database7"<<endl;
            }

            sqlite3_close(db);

            string result;
            stringstream resultss;

            resultss<<"OK "<<data_mail[0]<<" "<<data_mail[1]<<"B";
            result = resultss.str();
            return result;
        }


        string LIST(string username)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            pair<int,string> data_list;    // 0 -- no. of mails , 1 -- total size

            rc = sqlite3_open("Garuda.db", &db);

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"'";

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database8"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 0 ORDER BY EID DESC;";
            sql.clear();
            sql = sqlss.str();


            data_list.first = 0;
            (data_list.second).clear();
            rc = sqlite3_exec(db, &sql[0], callback_list, &data_list, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database9"<<endl;
            }

            sqlite3_close(db);

            string result;
            stringstream resultss;

            resultss<<"OK "<<data_list.first<<" messages\n"<<data_list.second;
            result = resultss.str();
            return result;
        }

        string RETR(string username,string id)
        {
            sqlite3 *db;
            char *zErrMsg = 0;
            int rc;
            string sql;
            int data[2];    // 0 -- UID , 1 -- SID            
            string data_retr;

            rc = sqlite3_open("Garuda.db", &db);

            sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+username+"'";

            rc = sqlite3_exec(db, &sql[0], callback, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database10"<<endl;
            }

            stringstream sqlss;

            sqlss<<"SELECT * FROM Email WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND RON = 0 AND EID = "<<id<<";";
            sql.clear();
            sql = sqlss.str();


            data_retr.clear();
            rc = sqlite3_exec(db, &sql[0], callback_retr, &data_retr, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database11"<<endl;
            }

            sqlss.str(std::string());
            sqlss<<"UPDATE Email SET RON = 1 WHERE Receiver = "<<data[0]<<" AND ReceiverServer = "<<data[1]<<" AND EID = "<<id<<";";
            sql.clear();
            sql = sqlss.str();

            rc = sqlite3_exec(db, &sql[0], callback_insert, &data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database111"<<endl;
            }


            sqlite3_close(db);

            if(data_retr.length() == 0)
            {
                string result = "ERR no such message";
                return result;
            }
            else return data_retr;   
        }

        string QUIT()
        {
            string result = "OK POP3 server signing off";
            return result;
        }

};


class Server_to_Server{
    public:
        void sendMsg(){
            uint32_t dataLength = htonl(sendBuf.size()); // Ensure network byte order
            // when sending the data length
            //send(connfd,&dataLength ,sizeof(uint32_t) ,MSG_CONFIRM); // Send the data length
            send(connfd,sendBuf.c_str(),sendBuf.size(),MSG_CONFIRM); // Send the string
            cout<<"Sent: "<<sendBuf<<endl;
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
            cout<<"Received: "<<msg<<endl;
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

        void sendMail(string mailFrom,string mailTo,string body){
            sendBuf.clear();
            string ack;

            // MAIL FROM
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

            sendBuf = "RCPT TO: "+mailTo;
            sendMsg();
            ack.clear();
            ack = receiveMsg();

            /*ack.clear();
            ack = receiveMsg();
            if(ack == "250 OK")
            {
                // continue
            }

            else ;// error*/

            
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
            sendBuf = body;
            sendMsg();
            ack.clear();
            ack = receiveMsg();
            if(ack == "250 OK")
            {
                // continue
            }
            else ;// error
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



SMTP_Server garudaSMTPServer;
POP_Server garudaPOPServer;


void* client_POP(void * args)
{
    int clientFD = *((int*)args);

    string username,password,retreive,reply,uid;

    garudaPOPServer.sendReady(clientFD);

    uid = garudaPOPServer.receiveMsg(clientFD);
    uid = uid.substr(4);
    garudaPOPServer.sendOK(clientFD);

    // Authorization State

    while(1)
    {
        username.clear();
        username = garudaPOPServer.receiveMsg(clientFD);
        if(username.substr(0,4) == "QUIT")
        {
            reply.clear();
            reply = garudaPOPServer.QUIT();
            garudaPOPServer.sendMsg(clientFD,reply);
            return NULL;
        }
    
        int res = garudaPOPServer.authorizeUser(username.substr(5),uid);
        if(res == 1)
        {
            garudaPOPServer.sendOK(clientFD);
            break;
        }
        else{
            garudaPOPServer.sendERR(clientFD);
            continue;
        }
    }

    while(1)
    {
        password.clear();
        password = garudaPOPServer.receiveMsg(clientFD);
        int res = garudaPOPServer.authorizePass(username.substr(5),password.substr(5));
        if(res == 1)
        {
            garudaPOPServer.sendOK(clientFD);
            break;
        }
        else{
            garudaPOPServer.sendERR(clientFD);
            continue;
        }   
    }

    // End Of Authorization State
    username = username.substr(5);
    // Transaction State
    while(1)
    {
        // receive request
        string req,id;
        req = garudaPOPServer.receiveMsg(clientFD);
        if(req.substr(0,5) == "STATR")
        {
            reply.clear();
            reply = garudaPOPServer.STAT_Read(username);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,5) == "LISTR")
        {
            reply.clear();
            reply = garudaPOPServer.LIST_Read(username);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,5) == "RETRR")
        {
            id.clear();
            reply.clear();
            id = req.substr(5);
            reply = garudaPOPServer.RETR_Read(username,id);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,4) == "STAT")
        {
            reply.clear();
            reply = garudaPOPServer.STAT(username);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,4) == "LIST")
        {
            reply.clear();
            reply = garudaPOPServer.LIST(username);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,4) == "RETR")
        {
            id.clear();
            reply.clear();
            id = req.substr(5);
            reply = garudaPOPServer.RETR(username,id);
            garudaPOPServer.sendMsg(clientFD,reply);
        }

        else if(req.substr(0,4) == "QUIT")
        {
            reply.clear();
            reply = garudaPOPServer.QUIT();
            garudaPOPServer.sendMsg(clientFD,reply);
            break;
        }
    }

    cout<<"POP3 Connection closed"<<endl;
}

void* client_SMTP(void * args)
{
    int clientFD = *((int*)args);

    string mailFrom,body,hostName;
    int sender,senderServer,receiver,receiverServer;
    vector<string> mailTo;
    multimap<string,string> mailTo_Forward;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    string sql;
    int data[2];    // 0 -- UID , 1 -- SID

    rc = sqlite3_open("Garuda.db", &db);
    if(rc)
    {
        cout<<"Can't open Database "<<endl;
    }
    else{
        cout<<"Database opened successfully"<<endl;
    }

    // Send 220 Service Ready

    garudaSMTPServer.send220(clientFD);

    string req;
    req = garudaSMTPServer.receiveMsg(clientFD);

    // get DNS Hostname
    hostName = req.substr(6);
    garudaSMTPServer.send250(clientFD);

    while(1)
    {

        // get source email address
        req.clear();
        req = garudaSMTPServer.receiveMsg(clientFD);

        if(req.substr(0,4) == "QUIT")
        {
            garudaSMTPServer.send221(clientFD);
            break;
        }

        else if(req.substr(0,4) == "VRFY")
        {
            garudaSMTPServer.handleVRFY(clientFD,req.substr(5));
            continue;
        }

        mailFrom = req.substr(11);
        garudaSMTPServer.send250(clientFD);

        mailTo.clear();
        mailTo_Forward.clear();

        int reset = 0;

        // get receipent
        while(1){
            req.clear();
            req = garudaSMTPServer.receiveMsg(clientFD);
            if(req.substr(0,7) == "RCPT TO"){

                string em_id = req.substr(9);
                string recServer;
                recServer.clear(); 
                int i = 0;
                while(em_id[i]!='@' && i<em_id.length())
                {
                    i++;
                }

                i++;
                int j = 0;
                while(i<em_id.length())
                {
                    recServer += em_id[i];
                    j++;
                    i++;
                }

                cout<<"recServer: "<<recServer<<"12"<<endl;
                
                // check user
                int data_rcpt = -1; 
                sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+req.substr(9)+"'";

                rc = sqlite3_exec(db, &sql[0], callback_rcpt, &data_rcpt, &zErrMsg);

                if(rc != SQLITE_OK)
                {
                    cout<<"Error Querying Database2"<<endl;
                }

                if(recServer != serverName)
                {
                    string sendBuf;
                    sendBuf.clear();
                    sendBuf = "251 User not local; will forward";
                    garudaSMTPServer.sendMsg(clientFD,sendBuf);
                    if(ServerPort.find(recServer) != ServerPort.end()) mailTo_Forward.insert(pair<string,string>(recServer,req.substr(9)));
                }

                else if(data_rcpt == -1)
                {
                    string sendBuf;
                    sendBuf.clear();
                    sendBuf = "550 No such user here";
                    garudaSMTPServer.sendMsg(clientFD,sendBuf);
                }

                else if(data_rcpt == serverID){
                    garudaSMTPServer.send250(clientFD);
                    mailTo.push_back(req.substr(9));
                }
            }
            else if(req.substr(0,4) == "DATA")
            {
                garudaSMTPServer.send354(clientFD);
                break;        
            }
            else if(req.substr(0,4) == "RSET")
            {
                garudaSMTPServer.send250(clientFD);
                reset = 1;
                break;
            }
        }

        if(reset == 0)
        {
            // get Body
            req.clear();
            req = garudaSMTPServer.receiveMsg(clientFD);
            body = req;
            garudaSMTPServer.send250(clientFD);

            // send Mail

            // If Receiver's Server is same as this server

            // Get Sender's Details

            /*sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+mailFrom+"'";

            // get SID and UID

            rc = sqlite3_exec(db, &sql[0], callback, data, &zErrMsg);

            if(rc != SQLITE_OK)
            {
                cout<<"Error Querying Database2"<<endl;
            }

            sender = data[0];
            senderServer = data[1];*/


            int i;

            for(i=0;i<mailTo.size();++i){
                // Get Receiver's Details

                sql = "SELECT * FROM IsUserOf WHERE email_addr = '"+mailTo[i]+"'";

                // get SID and UID

                rc = sqlite3_exec(db, &sql[0], callback, data, &zErrMsg);

                if(rc != SQLITE_OK)
                {
                    cout<<"Error Querying Database1"<<endl;
                }

                receiver = data[0];
                receiverServer = data[1];

                // If Receiver's Server is same as this server
                stringstream sqlss;
                sqlss<<"INSERT INTO Email(Sender,Receiver,ReceiverServer,Body,_Size) VALUES('"<<mailFrom<<"',"<<receiver<<","<<receiverServer<<",'"<<body<<"',"<<body.length()<<")";

                sql.clear();
                sql = sqlss.str();

                cout<<sql;

                rc = sqlite3_exec(db, &sql[0], callback_insert, data, &zErrMsg);

                if(rc != SQLITE_OK)
                {
                    cout<<"Error Querying Database3"<<endl;
                }

                else{
                    cout<<"Mail successfully sent to: "<<mailTo[i]<<endl;
                }
            }

            multimap<string,string>::iterator itr;
            itr = mailTo_Forward.begin();

            while(itr!= mailTo_Forward.end())
            {
                if(ServerPort.find(itr->first) != ServerPort.end()){
                    int PORT_NO_f = ServerPort[itr->first];
                    int clientFD_f = socket(AF_INET,SOCK_STREAM,0);
                    if(clientFD_f < 0)printf("Socket formation Failed\n");

                    struct sockaddr_in saddr_f;
                    printf("Tryint to Connect ...\n");

                    saddr_f.sin_family = AF_INET;
                    saddr_f.sin_port = htons(PORT_NO_f);

                    if(inet_pton(AF_INET,(ServerIP[itr->first]).c_str(),&saddr_f.sin_addr) <= 0)printf("Addr conv Failed\n");

                    if(connect(clientFD_f,(struct  sockaddr*)&saddr_f,sizeof(saddr_f)) < 0)printf("Connection Failed\n");

                    Server_to_Server serverServer;

                    serverServer.connfd = clientFD_f;

                    serverServer.connect();
                    serverServer.sendMail(mailFrom,itr->second,body);
                    serverServer.terminate();
                    itr++;
                }
            }
        }

        // else
    }

   /* else if(req.substr(0,4) == "QUIT")
    {
        garudaSMTPServer.send221(clientFD);
        garudaSMTPServer.clients.erase(clientFD);
    }*/
}


void init(char *IP,char *port)
{
    ServerIP.insert(pair<string,string>("abc.com","127.0.0.1"));
    ServerIP.insert(pair<string,string>("xyz.com",IP));

    ServerPort.insert(pair<string,int>("abc.com",5025));
    ServerPort.insert(pair<string,int>("xyz.com",atoi(port)));
}


int main(int argc, char * argv[])
{
    if(argc<5)
    {
        cout<<"USAGE: "<<argv[0]<<" PORT_SMTP PORT_POP OtherServer'sIP OtherServer'sPort"<<endl;
        cout<<"Try Again.."<<endl;
        exit(1);
    }
    init(argv[3],argv[4]);
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


    // POP SERVER
    int servfd_p = socket(AF_INET,SOCK_STREAM,0);
    int PORT_NO_p = atoi(argv[2]);
    printf("%d\n",servfd_p);
    char s_p[3000];

    if(servfd_p < 0)printf("Server socket could not be connected\n");
    //char  msg[MESSAGE_LENGTH];
    struct sockaddr_in saddr_p;
    saddr_p.sin_family = AF_INET;
    saddr_p.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr_p.sin_port = htons(PORT_NO_p);

    while(bind(servfd_p, (struct sockaddr *)&saddr_p,sizeof(saddr_p)) <0 ){
        printf("Bind Unsuccessful\n");
        cin>>PORT_NO_p;
        saddr_p.sin_port = htons(PORT_NO_p);
    }

    if(listen(servfd_p,NO_OF_CLIENTS) <0 )printf("Listen unsuccessful\n");


    int pzid = fork();

    if(pzid > 0)
    {
        // Handle SMTP Connections
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
                    pthread_create(&td,NULL,&client_SMTP,&connfd);

                }
                catch(const std::exception& e)
                {
                    cout<<e.what()<<endl;
                }
                //cout<<"Thread is terminated"<<endl;

            }
        }
    }

    else{
        // Handle POP Connections
        while(1)
        {
            int connfd_p = accept(servfd_p, (struct sockaddr*)NULL, NULL);
            if(connfd_p >= 0)
            {
                liveThreads++;

                pthread_t td_p;

                if(connfd_p<0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                cout<<"New Connection Started\tconnFD = "<<connfd_p<<endl;

                try
                {
                    pthread_create(&td_p,NULL,&client_POP,&connfd_p);

                }
                catch(const std::exception& e)
                {
                    cout<<e.what()<<endl;
                }
                //cout<<"Thread is terminated"<<endl;

            }
        }

    }

    return 0;
}