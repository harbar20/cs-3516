/***************************************************
 *  * file:     testpcap1.c
 *   * Date:     Thu Mar 08 17:14:36 MST 2001 
 *    * Author:   Martin Casado
 *     * Location: LAX Airport (hehe)
 *      *
 *       * Simple single packet capture program
 *        *****************************************************/
#include <iostream>
#include <fstream>
#include <pcap.h> /* if this gives you an error try pcap/pcap.h */
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <map>
#include <vector>
using namespace std;
struct host{
    int numPackets;
    char* macAddr;
    char* ipAddr;
};
struct pcapInfo {
    struct timeval startTime;
    int capDuration;
    int numPackets;
    map<char*,struct host> hosts;
    vector<char*> sourcePorts;
    vector<char*> destPorts;
    int minPackSize;
    int maxPackSize; 
};
int i = 0;
void TCPCallback (unsigned char *p, const struct pcap_pkthdr *h, const u_char *bytes) {
    struct timeval headerTime = h->ts;
    if(i < 10){
        //p->startTime = headerTime;
        struct ether_header *ethernetHeader = (struct ether_header *)bytes;
        cout << "Source port: ";
        for (int i = 0; i < sizeof(ethernetHeader->ether_shost); i++) {
            cout << (int) (ethernetHeader->ether_shost[i]) << ":";
        }
        cout << "\nDestination port: ";
        for (int i = 0; i < sizeof(ethernetHeader->ether_dhost); i++) {
            cout << (int) ethernetHeader->ether_dhost[i] << ":";
        }
        int protocolType = ntohs(ethernetHeader->ether_type);
        cout << "\nEthernet protocol type: " << protocolType << "\n";

        if(protocolType == ETHERTYPE_IP){
            struct ip *ipHeader = (struct ip *)(bytes + sizeof(struct ether_header));
            cout << "IP Protocol type: " << (int) ipHeader->ip_p << "\n";
            cout << "IP Source: " << inet_ntoa(ipHeader->ip_src) << "\n";
            cout << "IP Dest: " << inet_ntoa(ipHeader->ip_dst) << "\n";

            if(ipHeader->ip_p == IPPROTO_UDP){
                struct udphdr *udpHeader = (struct udphdr *)(bytes + sizeof(struct ether_header)+sizeof(struct ip));
                cout << "UDP Detected\n";
                cout << "Source port: " << ntohs(udpHeader->uh_sport) << "\n";
                cout << "Destination port: " << ntohs(udpHeader->uh_dport) << "\n";
            }
        }
        else if(protocolType == ETHERTYPE_ARP){
            cout << "ARP Detected" << "\n";
            //struct arp_ipv4 *arpHeader = (struct arp_ipv4 *)(bytes + sizeof(struct ether_header));
            //printf("ARP Source IP Address: %s\n", arpHeader->ar_sip);
            //printf("ARP Dest IP Address: %s\n", arpHeader->ar_tip);

        }
        i++;

        cout << "\nCapLen: " << h->caplen << "\n";
    }
}
int main(int argc, char **argv)
{
    int i;
    char *dev; 
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* descr;
    const u_char *packet;
    struct pcap_pkthdr hdr;     /* pcap.h */
    struct ether_header *eptr;  /* net/ethernet.h */
    int packetType;
    u_char *ptr; /* printing out hardware header info */

    /* grab a device to peak into... */
    dev = pcap_lookupdev(errbuf);

    if(dev == NULL)
    {
        cout << errbuf << "\n";
        exit(1);
    }

    cout << "DEV: " << dev << "\n";

    const char* packetFile = "project2-other-network.pcap";
    descr = pcap_open_offline(packetFile, errbuf);

    if(descr == NULL)
    {
        cout << "pcap_open_offline(): " << errbuf << "\n";
        exit(1);
    }
    packetType = pcap_datalink(descr);
    cout << "Packet Type: " << packetType << "\n";
    struct pcapInfo *p = (struct pcapInfo*) malloc(sizeof(struct pcapInfo));


    pcap_loop(descr,0,TCPCallback,NULL);

    return 0;
}

