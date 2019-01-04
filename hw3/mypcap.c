#include <stdio.h>
#include <pcap.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>

#define SIZE_ETHERNET 14
#define ETHER_ADDR_LEN 6

/* Ethernet header */
struct sniff_ethernet {
	u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
	u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
	u_short ether_type; /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
	u_char ip_vhl;		/* version << 4 | header length >> 2 */
	u_char ip_tos;		/* type of service */
	u_short ip_len;		/* total length */
	u_short ip_id;		/* identification */
	u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_char ip_ttl;		/* time to live */
	u_char ip_p;		/* protocol */
	u_short ip_sum;		/* checksum */
	struct in_addr ip_src;
	struct in_addr ip_dst; /* source and dest address */
};
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
	u_short th_sport;	/* source port */
	u_short th_dport;	/* destination port */
	tcp_seq th_seq;		/* sequence number */
	tcp_seq th_ack;		/* acknowledgement number */
	u_char th_offx2;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
	u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short th_win;		/* window */
	u_short th_sum;		/* checksum */
	u_short th_urp;		/* urgent pointer */
};

void printTime(int t){
	int year;	//year
	int month;	//month
	int day;	//day
	int hour;	//hour
	int min;	//minute
	int sec;	//second
	min=t/60;
	sec=t%60;
	hour=min/60;
	min=min%60;
	day=hour/24;
	hour=hour%24;
	month=day/30;
	day=day%30;
	year=month/12;
	month=month%12;
	
	printf("Epoch Time: %d years, %d months, %d days, %d hours, %d minutes, %d seconds\n", year, month, day, hour, min, sec);
}

int main(int argc, char *argv[]){
	if(argc!=3){
		perror("ERROR: Please input the filename and the restrictions(BPF)");
		return 0;
	}
	char *fname = argv[1];
	char *filter = argv[2];

	char errbuf[PCAP_ERRBUF_SIZE];    /* Error string */
	pcap_t *handle;    /* Session handle */

	/* Open File */
	handle=pcap_open_offline(fname,errbuf);
	if(!handle){
		perror("ERROR: Cannot open the file");
		return 0;
	}
	
	/* Set bpf */
	struct bpf_program fcode;
	if(-1 == pcap_compile(handle, &fcode, filter, 1, PCAP_NETMASK_UNKNOWN)){
		perror("ERROR: Cannot compile the file");
		pcap_close(handle);
		return 0;
	}
	if(strlen(filter) != 0){
		printf("Filter: %s\n", filter);
	}

	/* Read File */
	struct pcap_pkthdr *header = NULL;
	const u_char *packet = NULL;
	const struct sniff_ethernet *ethernet; /* The ethernet header */
	const struct sniff_ip *ip; /* The IP header */
	const struct sniff_tcp *tcp; /* The TCP header */
	const char *payload; /* Packet payload */
	u_int size_ip;
	u_int size_tcp;
	int count=0;

	while(pcap_next_ex(handle, &header, &packet)>=0){
		if(pcap_offline_filter(&fcode, header, packet)==0){
			perror("ERROR: File type doesn't mattch the filter");
			break;
		}
		printf("NO %i\n", ++count);
		printf("Size: %d bytes\n", header->len);
		printTime(header->ts.tv_sec);
		ethernet = (struct sniff_ethernet*)(packet);
		ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
		size_ip = IP_HL(ip)*4;
		tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);

		printf("src address: %s\ndest address: %s\n", inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_dst));
		printf("src port: %d\ndest port: %d\n", tcp->th_sport, tcp->th_dport);
		printf("----------------------------------------------\n");
	}
	return 0;
}

