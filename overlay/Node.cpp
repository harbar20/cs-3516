#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <queue>
#include <unistd.h>

using namespace std;

// Set the following port to a unique number:
#define MYPORT 5952
#define bufferlen 1000

struct overlayConfig
{
    int queueLen;
    int TTL;
    int nodeType;
    int ID;
    string actualIP;
    string overlayIP;
    string prefix;
    string routerIP;
};

enum StatusCode
{
    TTL_EXPIRED,
    MAX_SENDQ_EXCEEDED,
    NO_ROUTE_TO_HOST,
    SENT_OKAY
};

struct packet
{
    struct ip ipHeader;
    struct udphdr udpHeader;
    char data[bufferlen - sizeof(struct ip) - sizeof(struct udphdr)];
};

// Config parsed from the config file
struct overlayConfig config;
// List of all hashtables of different prefix lengths. Key: prefix, Value: router overlay IP
vector<map<string, string>> tableList;
// Delays from current node to all connected nodes. Key: node ID, Value: delay (ms)
map<int, int> delays;
// Contains all nodes in the network. Key: overlay IP, Value: node ID
map<string, int> nodes;

void parseConfig(char *filename, int nodeType, int id)
{
    ifstream infile(filename);
    int option;
    while (infile >> option)
    {
        switch (option)
        {
        case 0:
        {
            // code for config here
            infile >> config.queueLen >> config.TTL;
            break;
        }
        case 1:
        {
            int routerID;
            string routerIP;

            infile >> routerID;
            infile >> routerIP;

            nodes.insert({routerIP, routerID});

            if (option == nodeType)
            {
                if (routerID == id)
                {
                    config.ID = routerID;
                    config.actualIP = routerIP;
                }
            }

            break;
        }
        case 2:
        {
            int hostID;
            string hostRealIP;
            string hostOverlayIP;

            infile >> hostID >> hostRealIP >> hostOverlayIP;

            nodes.insert({hostOverlayIP, hostID});

            if (option == nodeType)
            {
                if (hostID == id)
                {
                    config.ID = hostID;
                    config.actualIP = hostRealIP;
                    config.overlayIP = hostOverlayIP;
                }
            }

            break;
        }
        case 3:
        {
            int routerID1;
            int delay1;
            int routerID2;
            int delay2;

            infile >> routerID1 >> delay1 >> routerID2 >> delay2;

            if (routerID1 == id)
            {
                delays.insert({routerID2, delay1});
            }
            else if (routerID2 == id)
            {
                delays.insert({routerID1, delay2});
            }

            break;
        }

        case 4:
        {
            int routerID;
            int routerDelay;
            string routerOverlayIP;
            int hostID;
            int hostDelay;

            infile >> routerID >> routerDelay >> routerOverlayIP >> hostID >> hostDelay;

            // Position of the slash separating the size of the prefix and the IP
            int slashPosition = routerOverlayIP.find_first_of("/");
            // Length of the prefix (right side of the "/")
            int prefixLength = stoi(routerOverlayIP.substr(slashPosition + 1));
            // Full router overlay IP address
            string overlayIP = routerOverlayIP.substr(0, slashPosition);

            if (routerID == id && nodeType == 1)
            {
                // Reaidng only the prefix of the router overlay IP address
                string prefix = "";
                int currentPrefixLength = 0;
                stringstream overlayStream(overlayIP);
                while (currentPrefixLength < prefixLength)
                {
                    string tempPrefix;
                    // Get everything up to next "."
                    getline(overlayStream, tempPrefix, '.');
                    // Increment counter to only include up to the prefixLength
                    currentPrefixLength += 8;
                    // Making sure not to add an extra "." at the end
                    if (currentPrefixLength < prefixLength)
                    {
                        tempPrefix.append(".");
                    }

                    // Appending everything to the prefix
                    prefix.append(tempPrefix);
                }

                map<string, string> tempMap;
                tempMap.insert({prefix, overlayIP});
                tableList.insert(tableList.begin() + prefixLength, tempMap);
                delays.insert({hostID, routerDelay});

                config.prefix = prefix;
                config.overlayIP = overlayIP;
            }
            else if (hostID == id && nodeType == 2)
            {
                delays.insert({routerID, hostDelay});
            }

            nodes.insert({overlayIP, routerID});
            config.routerIP = overlayIP;

            break;
        }

        default:
            break;
        }
    }
}

void DieWithError(string errorMessage)
{
    cout << errorMessage << endl;
    exit(1);
}; /* Error handling function */

void log(struct packet *p, int ipIdent, StatusCode code, string nextHop = "")
{
    FILE *fp = fopen("ROUTER_control.txt", "a+");

    // Getting current time in UNIX time
    time_t result = time(NULL);
    char *unixTime = asctime(localtime(&result));
    // Overlay IP's
    char *sourceIP = inet_ntoa(p->ipHeader.ip_src);
    char *destIP = inet_ntoa(p->ipHeader.ip_dst);
    char charNextHop[nextHop.length()];
    strcpy(charNextHop, nextHop.c_str());

    fprintf(fp, "%s %s %s %d %d %s\n", unixTime, sourceIP, destIP, ipIdent, code, charNextHop);

    fclose(fp);
}

int create_cs3516_socket()
{
    int sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
        DieWithError("Error creating CS3516 socket");

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(MYPORT);
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        DieWithError("Unable to bind CS3516 socket");

    // Socket is now bound:
    return sock;
}

int cs3516_recv(int sock, char *buffer, int buff_size)
{
    struct sockaddr_in from;
    unsigned int fromlen, n;
    fromlen = sizeof(struct sockaddr_in);
    n = recvfrom(sock, buffer, buff_size, 0,
                 (struct sockaddr *)&from, &fromlen);

    return n;
}

int cs3516_send(int sock, struct packet *buffer, int buff_size, unsigned long nextIP)
{
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

void sendPacket(int sock, char *buffer, char *ipAddress)
{
    // Building the IP header
    struct ip ipHeader;
    string sipAddr = config.overlayIP;
    string dipAddr = ipAddress;
    inet_aton(sipAddr.c_str(), &ipHeader.ip_src);
    inet_aton(dipAddr.c_str(), &ipHeader.ip_dst);
    ipHeader.ip_p = IPPROTO_UDP;
    ipHeader.ip_ttl = config.TTL;
    ipHeader.ip_hl = 5;
    ipHeader.ip_tos = 0;
    ipHeader.ip_off = 0;
    ipHeader.ip_sum = 0;

    // Building the UDP header
    struct udphdr udpHeader;
    udpHeader.uh_sport = MYPORT;
    udpHeader.uh_dport = MYPORT;
    udpHeader.uh_sum = 0;

    // Building the data of the packet
    string data = to_string(rand());

    // Building the final packet
    struct packet p;
    p.ipHeader = ipHeader;
    p.udpHeader = udpHeader;
    strcpy(p.data, data.c_str());

    // Sending the packet to this host's router
    if (cs3516_send(sock, &p, sizeof(struct packet), inet_addr(config.routerIP.c_str())) <= 0)
    {
        DieWithError("Failed to send packet");
    }

    cout << "Sent packet as follows: " << endl;
    cout << "   From: " << inet_ntoa(p.ipHeader.ip_src) << endl;
    cout << "   To: " << inet_ntoa(p.ipHeader.ip_dst) << endl;
    cout << "   Contains: " << p.data << endl;
}

struct packet *receivePacket(int sock, char *buffer)
{
    if (cs3516_recv(sock, buffer, bufferlen) <= 0)
    {
        DieWithError("Failed to receive packet");
    }
    struct packet *p = (struct packet *)buffer;
    struct ip ipHeader = p->ipHeader;
    struct udphdr udpHeader = p->udpHeader;

    cout << "Sent packet as follows: " << endl;
    cout << "   From: " << inet_ntoa(p->ipHeader.ip_src) << endl;
    cout << "   To: " << inet_ntoa(p->ipHeader.ip_dst) << endl;
    cout << "   Contains: " << p->data << endl;

    return p;
}

int main(int argc, char **argv)
{
    // Parsing arguments
    if (argc != 4)
    {
        cout << "Usage: ./node [config file name] [1 for router, 2 for host] [node ID]";
        exit(1);
    }
    char *fileName = argv[1];
    int nodeType = atoi(argv[2]);
    int nodeID = atoi(argv[3]);

    // Setting up
    // un-seeding any RNG
    srand(time(NULL));
    // allocating space for all the IP prefixes
    // making it 33 to allow space for a full-IP prefix
    tableList.resize(33);
    // parsing config file
    parseConfig(fileName, nodeType, nodeID);
    // creating socket
    int sock = create_cs3516_socket();

    // This node is a router
    if (nodeType == 1)
    {
        // Packet queue for routers
        queue<struct packet *> pq;

        while (true)
        {
            char buffer[bufferlen];

            // Select setup to have router route packets only if
            // something has been sent to the socket
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            int recVal = select(sock + 1, &rfds, NULL, NULL, NULL);

            // Checking the constantly-changing return value of the select function
            switch (recVal)
            {
            case (0):
            {
                // Only if a packet is queued
                if (pq.size() > 0)
                {
                    // Reading the first packet in the queue, then removing it
                    struct packet *tempP = pq.front();
                    pq.pop();

                    char *destIP = inet_ntoa(tempP->ipHeader.ip_dst);

                    // Trying to find the prefix for the destination IP
                    string destPrefix = "";
                    int destPrefixLength = 0;
                    // Building the prefix up to the given length
                    stringstream destIPStream(destIP);
                    while (destPrefixLength < tableList.size())
                    {
                        string tempPrefix;
                        // Get everything up to next "."
                        getline(destIPStream, tempPrefix, '.');

                        // Check if entry exists for calculated prefix so far
                        if (tableList[destPrefixLength].count(destPrefix))
                        {
                            break;
                        }
                        // Increment counter to only include up to the max
                        destPrefixLength += 8;
                        // Adding dots between parts of the IP
                        tempPrefix.append(".");

                        // Appending everything to the prefix
                        destPrefix.append(tempPrefix);
                    }

                    // If no valid prefix was found
                    if (destPrefix.empty())
                    {
                        log(tempP, nodeType, NO_ROUTE_TO_HOST);
                    }
                    else
                    {
                        // IP of next hop node
                        string nextHopIP = tableList[destPrefixLength].at(destPrefix);
                        // Checking if nextHop that is found from tableList is the same as the current node
                        // i.e. if the current node is the last router before the destination
                        if (strcmp(nextHopIP.c_str(), config.overlayIP.c_str()) == 0)
                        {
                            nextHopIP = destIP;
                        }
                        // ID of next hop node
                        int nextHopID = nodes.at(nextHopIP);
                        // Applying delay for next hop
                        sleep((float) delays.at(nextHopID)/1000);

                        // Forwarding packet to nextHop
                        if (cs3516_send(sock, tempP, sizeof(struct packet), inet_addr(nextHopIP.c_str())) <= 0)
                        {
                            DieWithError("Failed to send packet");
                        }

                        log(tempP, nodeType, SENT_OKAY, nextHopIP);
                    }
                }

                break;
            }
            case (-1):
            {
                DieWithError("Select returned -1.");
                break;
            }
            default:
            {
                struct packet *changeP = receivePacket(sock, buffer);
                changeP->ipHeader.ip_ttl--;

                // Adding packet to queue only if TTL is not 0
                if (changeP->ipHeader.ip_ttl == 0)
                {
                    log(changeP, nodeType, TTL_EXPIRED);
                }
                else
                {
                    if (pq.size() <= config.queueLen)
                        pq.push(changeP);
                    else
                        log(changeP, nodeType, MAX_SENDQ_EXCEEDED);
                }

                break;
            }
            }
        }
    }
    // This node is a host
    else if (nodeType == 2)
    {
        char buffer[bufferlen];
        sleep(.01);
        sendPacket(sock, buffer, argv[1]);

        for (;;)
        {
            char buffer[bufferlen];
            struct packet *p = receivePacket(sock, buffer);
            p->ipHeader.ip_ttl--;
        }
    }
}