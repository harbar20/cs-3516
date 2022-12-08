#include <iostream>
#include <fstream>
#include <pcap.h> /* if this gives you an error try pcap/pcap.h */
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <bits/stdc++.h>
using namespace std;

struct host{ //struct containing the mac address, ip address, and number of packets sent or received for each host
    int packets;
    string macAddr;
    string ipAddr;
};

struct pcapInfo { //struct containing all the information about the packet to be printed
    struct timeval startTime; //packet start time
    double capDuration; //amount of time taken to capture every packet
    tm* startHour; //Holds all of tv_sec from startTime
    int numPackets; //total number of packets
    map<string, struct host> senders; //hashmaps of all the hosts that sent or received a packet using the mac address as the key
    map<string, struct host> receivers;
    vector<string> sourcePorts; //vectors containing the UDP source and destination ports
    vector<string> destPorts;
    int minPackSize; //minimum and maximum packet sizes
    int maxPackSize;
    int packTotal; //overall total number of bytes in the captured packets

    pcapInfo() { //set minPackSize to the max int so every packet size will be less than or equal to it
        minPackSize = 2147483647;
        numPackets = 0; 
        packTotal = 0;
    }
};

// Global variable keeping track of combined stats from all packets in packet capture
struct pcapInfo pi;

// Callback function for pcap_loop that parses the headers of the packets
void Callback (unsigned char *p, const struct pcap_pkthdr *h, const u_char *bytes) {
    int currLen = h->len-16; //subtract 16 bytes from the packet length, representing the tcpdump
    pi.numPackets++;
    pi.packTotal += currLen;
    // Determine packet capture duration in seconds by converting the tv_sec into microseconds, 
    // adding them to tv_usec, and converting final result back to seconds
    pi.capDuration = ((h->ts.tv_sec * 1000000) + h->ts.tv_usec) - ((pi.startTime.tv_sec * 1000000) + pi.startTime.tv_usec);
    pi.capDuration /= 1000000;     

    // If it's the first packet, find the start time and date
    if (pi.numPackets == 1) {
        pi.startTime = h->ts;
        pi.startHour = localtime(&pi.startTime.tv_sec);
    }
    struct ether_header *ethernetHeader = (struct ether_header *)bytes;

    string srcAddr = "";

    //Finds mac address one number at a time and appends to a string
    for (int i = 0; i < sizeof(ethernetHeader->ether_shost); i++) {
        srcAddr += to_string((int) ethernetHeader->ether_shost[i]);
        if (i != sizeof(ethernetHeader->ether_shost) - 1) 
            srcAddr += ":";
    }
    //add address to senders hashmap if not already there, and increment number of packets sent by 1
    if (pi.senders.count(srcAddr) == 0) {
        struct host srcHost;
        srcHost.packets = 1;
        srcHost.macAddr = srcAddr;
        pi.senders.insert({srcAddr, srcHost});
    }
    else if (pi.senders.count(srcAddr) > 0) {
        pi.senders[srcAddr].packets++;
    }

    //Finds mac address one number at a time and appends to a string
    string destAddr = "";
    for (int i = 0; i < sizeof(ethernetHeader->ether_dhost); i++) {
        destAddr += to_string((int) ethernetHeader->ether_dhost[i]);
        if (i != sizeof(ethernetHeader->ether_dhost) - 1)
            destAddr += ":";
    }

    //add address to receivers hashmap if not already there, and increment number of packets sent by 1
    if (pi.receivers.count(destAddr) == 0) {
        struct host destHost;
        destHost.packets = 1;
        destHost.macAddr = destAddr;
        pi.receivers.insert({destAddr, destHost});
    } else if (pi.receivers.count(destAddr) > 0) {
        pi.receivers[destAddr].packets++;
    }

    int protocolType = ntohs(ethernetHeader->ether_type);
    // Checks if protocol is IP or ARP
    if(protocolType == ETHERTYPE_IP){
        struct ip *ipHeader = (struct ip *)(bytes + sizeof(struct ether_header));

        // adds ip addresses to hashmap
        pi.senders[srcAddr].ipAddr = inet_ntoa(ipHeader->ip_src);
        pi.receivers[destAddr].ipAddr = inet_ntoa(ipHeader->ip_dst);

        // Checks if UDP is used, and adds to UDP ports to source or destination port vectors
        if(ipHeader->ip_p == IPPROTO_UDP) {
            struct udphdr *udpHeader = (struct udphdr *)(bytes + sizeof(struct ether_header) + sizeof(struct ip));
            string sourcePort = to_string(ntohs(udpHeader->uh_sport));
            if (count(pi.sourcePorts.begin(), pi.sourcePorts.end(), sourcePort) == 0)
                pi.sourcePorts.push_back(sourcePort);
            string destPort = to_string(ntohs(udpHeader->uh_dport));
            if (count(pi.destPorts.begin(), pi.destPorts.end(), destPort) == 0)
                pi.destPorts.push_back(destPort);
        }
    }
    // adds hosts with ARP to arpList
    else if(protocolType == ETHERTYPE_ARP){
        struct ether_arp *arpHeader = (struct ether_arp *)(bytes + sizeof(struct ether_header));
        struct in_addr *srcIp = (struct in_addr *) arpHeader->arp_spa;
        struct in_addr *destIp = (struct in_addr *) arpHeader->arp_tpa;

    }
    // finds max and min packet values
    if (currLen < pi.minPackSize) {
        pi.minPackSize = currLen;
    }
    if (currLen > pi.maxPackSize) {
        pi.maxPackSize = currLen;
    }
}
int main(int argc, char **argv)
{
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr; //file pointer for the packets
    struct pcap_pkthdr hdr;
    struct ether_header *eptr;
    int packetType;
    u_char *ptr;

    // Sets up error buffer in case file not found
    dev = pcap_lookupdev(errbuf);

    if(dev == NULL)
    {
        cout << errbuf << "\n";
        exit(1);
    }

    cout << "DEV: " << dev << "\n";

    // Open packet file
    const char* packetFile = argv[1];

    descr = pcap_open_offline(packetFile, errbuf);

    if(descr == NULL)
    {
        cout << "pcap_open_offline(): " << errbuf << "\n";
        exit(1);
    }
    packetType = pcap_datalink(descr);

    // Parse packet data
    pcap_loop(descr,0,Callback,NULL);

    // Prints all the packet information gathered from pcap_loop
    cout << "Start Date: " << put_time(pi.startHour, "%d-%m-%Y %H:%M:%S ") << pi.startTime.tv_usec << "us" << endl;
    cout << "Packet Capture Duration: " << pi.capDuration << "sec\n";
    cout << "Number of Packets: " << pi.numPackets << endl;
    cout << "Senders: [Mac Addr][Ip Addr][Packets Sent]" << endl;

    // for loops printing the addresses and number of packets for the senders and receivers hash maps
    for (auto const& imap: pi.senders){
        string key = imap.first;
        struct host val = pi.senders[key];
        cout << "    " << val.macAddr << ", " << val.ipAddr << ", " << val.packets << endl;
    }
    cout << "Recipients: [Mac Addr][Ip Addr][Packets Received]" << endl;
    for (auto const& imap: pi.receivers){
        string key = imap.first;
        struct host val = pi.receivers[key];
        cout << "    " << val.macAddr << ", " << val.ipAddr << ", " << val.packets << endl;
    }
    cout << endl;
    
    vector<string>::iterator ip;

    // Printing UDP ports
    cout << "Unique Source Ports: " << endl;
    for (ip = pi.sourcePorts.begin(); ip != pi.sourcePorts.end(); ++ip) {
        cout << "    " << *ip << endl;
    }
    cout << "Unique Destination Ports: " << endl;
    for (ip = pi.destPorts.begin(); ip!= pi.destPorts.end(); ++ip) {
        cout << "    " << *ip << endl;
    }
    cout << endl;

    //Prints min, max, and average packet size
    cout << "Min Packet Size: " << pi.minPackSize << endl;
    cout << "Max Packet Size: " << pi.maxPackSize << endl;
    cout << "Avg Packet Size: " << pi.packTotal/pi.numPackets << endl;
    return 0;
}

