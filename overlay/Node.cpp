#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <netinet/ip.h>
#include <netinet/udp.h>

using namespace std;

// Set the following port to a unique number:
#define MYPORT 5950

#define bufferlen 1000
struct packet {
    struct ip ipHeader;
    struct udphdr udpHeader;
    char data[1000-sizeof(struct ip) - sizeof(struct udphdr)];
};

void DieWithError(string errorMessage)
{
	cout << errorMessage << endl;
	exit(1);
}; /* Error handling function */

int create_cs3516_socket() {
    int sock;
    struct sockaddr_in server;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) DieWithError("Error creating CS3516 socket");

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(MYPORT);
    if (bind(sock, (struct sockaddr *) &server, sizeof(server) ) < 0) 
        DieWithError("Unable to bind CS3516 socket");

    // Socket is now bound:
    return sock;
}

int cs3516_recv(int sock, char *buffer, int buff_size) {
    struct sockaddr_in from;
    unsigned int fromlen, n;
    fromlen = sizeof(struct sockaddr_in);
    n = recvfrom(sock, buffer, buff_size, 0,
                 (struct sockaddr *) &from, &fromlen);

    return n;
}

int cs3516_send(int sock, struct packet *buffer, int buff_size, unsigned long nextIP) {
    struct sockaddr_in to;
    int tolen, n;

    tolen = sizeof(struct sockaddr_in);

    // Okay, we must populate the to structure. 
    bzero(&to, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(MYPORT);
    to.sin_addr.s_addr = nextIP;

    // We can now send to this destination:
    n = sendto(sock, buffer, buff_size, 0, (struct sockaddr *)&to, tolen);

    return n;
}


void sendPacket(int sock, char* buffer, char* ipAddress){
    struct ip ipHeader;
    struct in_addr tempAddr;
    struct udphdr udpHeader;

    string sipAddr = "10.63.39.1";
    string dipAddr = "10.63.39.1";
    char sipArray[sipAddr.length() + 1];
    char dipArray[dipAddr.length() + 1];
    strcpy(sipArray, sipAddr.c_str());
    strcpy(dipArray, dipAddr.c_str());

    inet_aton(sipAddr.c_str(),&ipHeader.ip_src);
    inet_aton(dipAddr.c_str(),&ipHeader.ip_dst);
    ipHeader.ip_p = IPPROTO_UDP;
    ipHeader.ip_ttl = 2;
    ipHeader.ip_hl = 5;
    ipHeader.ip_tos = 0;
    ipHeader.ip_off = 0;
    ipHeader.ip_sum = 0;

    udpHeader.uh_sport = MYPORT;
    udpHeader.uh_dport = MYPORT;
    udpHeader.uh_sum = 0;

    char data[7] = "hello";

   // ipHeader.ip_len = strlen((char*)&p);
   // udpHeader.uh_ulen = strlen((char*)&p) - ipHeader.ip_hl;

    struct packet p;
    p.ipHeader = ipHeader;
    p.udpHeader = udpHeader;
    strcpy(p.data, data);

    cout << inet_ntoa(p.ipHeader.ip_src) << endl;
    cout << inet_ntoa(p.ipHeader.ip_dst) << endl;
    cout << p.data << endl;

    if(cs3516_send(sock,&p,sizeof(struct packet),inet_addr(ipAddress))<=0){
        DieWithError("Failed to send packet");
    }
}

struct packet* receivePacket(int sock, char* buffer){
    if(cs3516_recv(sock,buffer,1000)<=0){
        DieWithError("Failed to receive packet");
    }
    struct packet* ap = (struct packet*)buffer;
    struct ip testIp = ap->ipHeader;
    struct udphdr testUdp = ap->udpHeader;

    cout << inet_ntoa(testIp.ip_src) << endl;
    cout << inet_ntoa(testIp.ip_dst) << endl;



    cout << "before test data accessed" << endl;
    //char *testData = ap->data;
    //cout << "after test data accessed" << endl;
    //string s = testData;
    //if(s == NULL){
    //	DieWithError("No data\n");
    //}
    //cout << s << endl;
    cout << ap->data << endl;
    
    //struct ip* testIp = (struct ip*) buffer;
    //cout << "TestIP defined" << endl;
    //struct udphdr* testUdp = (struct udphdr*) (buffer + sizeof(struct ip*));
    //char *testData = buffer + sizeof(struct ip*) + sizeof(struct udphdr*);
    cout << inet_ntoa(testIp.ip_src) << endl;
    cout << inet_ntoa(testIp.ip_dst) << endl;
    return ap;

}


int main(int argc, char** argv){
    int sock = create_cs3516_socket();
    int nodeType = atoi(argv[2]);
    if(nodeType == 1){
        for (;;){
	        char buffer[1000];
	        struct packet* changeP = receivePacket(sock,buffer);
            //if(cs3516_send(sock, &changeP, sizeof(changeP), inet_addr(argv[1]))<=0){
            //    DieWithError("Failed to send packet");
            //}
        }

    }else if(nodeType == 2){
        char buffer[1000];
	sendPacket(sock, buffer, argv[1]);
        /*for (;;){
	        char buffer[1000];
	        struct packet* changeP = receivePacket(sock,buffer);
            changeP = editBuffer(changeP, sock);
            if(cs3516_send(sock, &changeP, sizeof(changeP), inet_addr(argv[1]))<=0){
                DieWithError("Failed to send packet");
            }
        }
        */
    }
    exit(0);
}
