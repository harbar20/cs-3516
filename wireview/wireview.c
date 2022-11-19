/***************************************************
 * file:     testpcap1.c
 * Date:     Thu Mar 08 17:14:36 MST 2001 
 * Author:   Martin Casado
 * Location: LAX Airport (hehe)
 *
 * Simple single packet capture program
 *****************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h> /* if this gives you an error try pcap/pcap.h */
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <netinet/ip.h>
#include <netinet/udp.h>
struct pcapInfo {
    struct timeval startTime;
    bpf_u_int32 bytesRead;
};

//Ethernet ARP packet from RFC 826 
//struct arp_ether_ipv4 {
//   uint16_t ar_hrd;		/* Format of hardware address */
// uint16_t ar_pro;		/* Format of protocol address */
// uint8_t ar_hln;		/* Length of hardware address */
// uint8_t ar_pln;		/* Length of protocol address */
// uint16_t ar_op;		/* ARP opcode (command) */
// uint8_t ar_sha[6];	/* Sender hardware address */
// uint32_t ar_sip;		/* Sender IP address */
// uint8_t ar_tha[6];	/* Target hardware address */
// uint32_t ar_tip;		/* Target IP address */
//};

void TCPCallback (struct pcapInfo *p, const struct pcap_pkthdr *h, const u_char *bytes) {
    struct timeval headerTime = h->ts;
    //if(p->bytesRead == 0){
    p->startTime = headerTime;
    struct ether_header *ethernetHeader = (struct ether_header *)bytes;
    printf("Source port: ");
    for (int i = 0; i < sizeof(ethernetHeader->ether_shost); i++) {
        printf("%d ", ethernetHeader->ether_shost[i]);
    }
    printf("\n");
    printf("Destination port: ");
    for (int i = 0; i < sizeof(ethernetHeader->ether_dhost); i++) {
        printf("%d ", ethernetHeader->ether_dhost[i]);
    }
    printf("\n");
    int protocolType = ntohs(ethernetHeader->ether_type);
    printf("Protocol type: %d\n", protocolType);

    printf("\n");
    if(protocolType == ETHERTYPE_IP){
        struct ip *ipHeader = (struct ip *)(bytes + sizeof(struct ether_header));
        printf("IP Protocol type: %d\n", ipHeader->ip_p);
        printf("IP Source: %s\n", inet_ntoa(ipHeader->ip_src));
        printf("IP Dest: %s\n", inet_ntoa(ipHeader->ip_dst));

        if(ipHeader->ip_p == IPPROTO_UDP){
            struct udphdr *udpHeader = (struct udphdr *)(bytes + sizeof(struct ether_header)+sizeof(struct ip));
            printf("UDP Detected\n");
            printf("Source port: %d\n",ntohs(udpHeader->uh_sport));
            printf("Destination port: %d\n",ntohs(udpHeader->uh_dport));
        }
        //  }
        /*
           else if(protocolType == ETHERTYPE_ARP){
           struct arp_ipv4 *arpHeader = (struct arp_ipv4 *)(bytes + sizeof(struct ether_header));
           printf("ARP Source IP Address: %s\n", arpHeader->ar_sip);
           printf("ARP Dest IP Address: %s\n", arpHeader->ar_tip);

           }
           */
    }

    p->bytesRead += h->caplen;

    printf("\nCapLen: %d\n", h->caplen);
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
        printf("%s\n",errbuf);
        exit(1);
    }

    printf("DEV: %s\n",dev);

    const char* packetFile = "project2-dns.pcap";
    descr = pcap_open_offline(packetFile, errbuf);

    if(descr == NULL)
    {
        printf("pcap_open_offline(): %s\n",errbuf);
        exit(1);
    }
    packetType = pcap_datalink(descr);
    printf("Packet Type: %d\n",packetType);
    struct pcapInfo *p = malloc(sizeof(struct pcapInfo));
    p->bytesRead = 0;


    pcap_loop(descr,0,TCPCallback,p);

    return 0;
}
