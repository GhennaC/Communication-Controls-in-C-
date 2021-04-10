#include "skel.h"

//structurile de route_table_entry si arp_entry implementate in laborator.
typedef struct rtable{
	uint32_t pref;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
}R_table;

typedef struct arp_entry{
	__u32 ip;
	uint8_t mac[6];
}E_arp;

struct rtable *tabl;
struct arp_entry *arp;
int size_arptable,size_rtable;

/*functia de ip_checksum fiind implementata in laborator,citind pe forum
am gasit ca se poate lua din laborator.*/
uint16_t ip_checksum(void* vdata,size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data=(char*)vdata;

	// Initialise the accumulator.
	uint64_t acc=0xffff;

	// Handle any partial block at the start of the data.
	unsigned int offset=((uintptr_t)data)&3;
	if (offset) {
		size_t count=4-offset;
		if (count>length) count=length;
		uint32_t word=0;
		memcpy(offset+(char*)&word,data,count);
		acc+=ntohl(word);
		data+=count;
		length-=count;
	}

	// Handle any complete 32-bit blocks.
	char* data_end=data+(length&~3);
	while (data!=data_end) {
		uint32_t word;
		memcpy(&word,data,4);
		acc+=ntohl(word);
		data+=4;
	}
	length&=3;

	// Handle any partial block at the end of the data.
	if (length) {
		uint32_t word=0;
		memcpy(&word,data,length);
		acc+=ntohl(word);
	}

	// Handle deferred carries.
	acc=(acc&0xffffffff)+(acc>>32);
	while (acc>>16) {
		acc=(acc&0xffff)+(acc>>16);
	}

	// If the data began at an odd byte address
	// then reverse the byte order to compensate.
	if (offset&1) {
		acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}

/*Citirea in tabela de tip rtable si arp_entry(Parsarea) */
int read_rtable(FILE *fd){
	char s1[70],s2[70],s3[70];
	int interf;
	int size = 100;
	int i = 0;
	while((fscanf(fd,"%s %s %s %d",s1,s2,s3,&interf)) != EOF ){
		if(i == size - 1 ){
			size += 100;
			tabl = realloc(tabl,size * sizeof(struct rtable));
		}
		inet_pton(AF_INET,s1,&tabl[i].pref);
		inet_pton(AF_INET,s2,&tabl[i].next_hop);
		inet_pton(AF_INET,s3,&tabl[i].mask);
		tabl[i].interface = interf;
		i++;
	}
	return i;
}

int read_arptable(FILE *fs){
	char s1[40],s2[40];
	int size = 5;
	int i = 0;
	while((fscanf(fs,"%s %s",s1,s2)) != EOF){
		if(i == size - 1){
			size += 10;
			arp = realloc(arp,size * sizeof(struct arp_entry));
		}
		inet_pton(AF_INET,s1,&arp[i].ip);
		hwaddr_aton(s2,arp[i].mac);
		i++;
	}
	return i;
}

//Comparator pentru functia qsort,pentru ordonarea tabelei rtable.
//Prefix crescator,masca descrescator.
int cmpQsort(const void *a,const void *b){
	const R_table *first = (R_table *) a;
	const R_table *second = (R_table *) b;
	if(first->pref == second->pref){
		if(first->mask > second->mask){
			return -1;
		}
		else return 1;
	}
	return (first->pref - second->pref);
}

/*Cautarea celei mai bune route,implementata cu binary search
se gaseste primul element cu prefixul cautat(dupa formula),apoi
se verifica daca mai exista valori cu acelasi prefix si mac mai mare 
deasupra sa,incrementand pana ajunge la ultima valoare cu cel mai mare mac.*/
R_table *get_best_route(__u32 caut){
	int i = 0, j = size_rtable;
	int m = (i+j)/2;
	while(i<j && (tabl[m].mask & caut) != tabl[m].pref){
		if((tabl[m].mask & caut) < tabl[m].pref)
			j = m-1;
		else i = m+1;	

		m = (i+j)/2;
	}

	if((tabl[m].mask & caut) == tabl[m].pref){
		if(tabl[m].pref == tabl[m-1].pref){
			while(tabl[m].pref == tabl[m-1].pref){
				m--;
			}
		}
		return &tabl[m];
	}
	return 	NULL;
}

E_arp *get_arp(__u32 ip){
	int index = -1;
	for(int i = 0 ; i < size_arptable;i++){
		if(ip == arp[i].ip)
			index = i;
	}
	if(index == -1)
		return NULL;
	return &arp[index];
}
/*Functie utilizata la implementarea suportului pentru protocolul ICMP.
 Se schimba valori din structurile eth_hdr,ip_hdr si icmp hdr,
 src si dest -> adresele primite din ip_hdr la primirea pachetului
 router mac si src mac -> mac-urile primite din eth
 ambele perechi sunt date ca parametri pentru a le interschimba intre ele.
 Se seteaza valorile head-erului iphdr si icmphdr dupa cum am facut si 
 la laborator.
 */
void init_hdrs(struct ether_header *eth_hd,struct iphdr *ip_hdr,struct icmphdr *icmp_hdr,__u32 src,__u32 dest,
	uint8_t router_mac[],uint8_t src_mac[]){
	for(int i = 0 ; i < 6 ; i++){
		eth_hd->ether_shost[i] = router_mac[i];
		eth_hd->ether_dhost[i] = src_mac[i];
	}
	ip_hdr->version = 4;
		ip_hdr->ihl = 5;
		ip_hdr->tos = 0;
		ip_hdr->id = htons(1);
		ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
		ip_hdr->frag_off = 0;
		ip_hdr->protocol = IPPROTO_ICMP;
		ip_hdr->ttl = 64;
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
		ip_hdr->daddr = src;
		ip_hdr->saddr = dest;

		icmp_hdr->code = 0;
		icmp_hdr->un.echo.id = htons(getpid());
		icmp_hdr->checksum = 0;
		icmp_hdr->checksum = ip_checksum(icmp_hdr,(sizeof(struct icmphdr)));

}
/*Pentru conditiile ttl<= 1 si inexistenta unei rute 
am ales sa creez un pachet nou,astfel fiind create cele 3 headere in functie 
si apoi apelata functia init_hdrs.
ip_src,ip_dest->adresele primite din ip_hdr la primirea pachetului;
routermac,src,mac->mac-urile primite din eth al pachetului primit;
type->tipul headerului ICMP
*/
void init_pack (packet *repl , __u32 ip_src , __u32 ip_dest,uint8_t router_mac[], uint8_t src_mac[],int type){
	repl->len = sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct icmphdr);
	struct ether_header *eth_hd = (struct ether_header *)repl->payload;
	struct iphdr *ip_hd = (struct iphdr *)(repl->payload + sizeof(struct ether_header));
	struct icmphdr *icmp_hd = (struct icmphdr *)(repl->payload + sizeof(struct ether_header)+ sizeof(struct iphdr));
	icmp_hd->type = type;

	eth_hd->ether_type = htons(ETHERTYPE_IP);

	init_hdrs(eth_hd,ip_hd,icmp_hd,ip_src,ip_dest,router_mac,src_mac);
}


int main(int argc, char *argv[]){
	tabl = (struct rtable*)malloc(100 * sizeof(struct rtable));
	DIE((tabl == NULL),"mem");
	packet m;
	int rc;	

	//Citirea(parsarea) tabelelor arp si route 
	FILE *fd = fopen("rtable.txt","r");
	DIE((fd == NULL),"rtable not opened");
	size_rtable = read_rtable(fd);
	

	FILE *fs = fopen("arp_table.txt","r");
	DIE((fs == NULL),"ar_table not opened");
	arp = (struct arp_entry*)malloc(5 * sizeof(struct arp_entry));
	size_arptable = read_arptable(fs);

	fclose(fd);
	fclose(fs);
	
	//Sortarea se face pentru obtinerea unei complexitati de O(logn)
	//la momentul cautarii celei mai bune rute.
	qsort(tabl,size_rtable,sizeof(struct rtable),cmpQsort);

	 init();

	 /*La fiecare pachet primit se verifica checksum-ul,ttl si existenta rutei
	 	daca pachetul nu trece de aceste if-uri -> este aruncat si se transmite
	 	mesaj send-erului conditia nerespectata,altfel
	 	se verifica daca este un packet obisnuit de redirectionat sau un
	 	packet cu icmp-type == icmp echo.
	 	Forwarding-ul a fost implementat avand ca schelet laboratorul04.
	 	*/
	  while (1) {
		rc = get_packet(&m);
	  	DIE(rc < 0, "get_message");
	  	struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = (struct icmphdr*)(m.payload + sizeof(struct ether_header)+ sizeof(struct iphdr));

		if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0) {
			printf("Wrong checksum\n"); 
			continue;  
		}

		if (ip_hdr->ttl <= 1) {
			packet repl;
			init_pack(&repl,ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_dhost,eth_hdr->ether_shost,11);
			send_packet(m.interface,&repl); 
			printf("Time to live!\n");
			continue;
		}
	 	
	 	struct rtable *rtabl = get_best_route(ip_hdr->daddr);
	 	if(rtabl == NULL){
			packet repl;
			init_pack(&repl,ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_dhost,eth_hdr->ether_shost,3);
			send_packet(m.interface , &repl); 
			printf("No route found!\n");
			continue;
	 	}

	 	//la primirea unui icmp cu type icmp echo
	 	//se verifica daca ipdest este unul dintre 
	 	//ip-urile interfetelor noastre in caz ca 
	 	//exista se modifica pachetul si se trimite.
	 	if(icmp_hdr->type == ICMP_ECHO){
	 		int contor = 0;
	 		for(int i = 0; i <= 3 ; i++){
	 			__u32 search;
	 			inet_pton(AF_INET,get_interface_ip(i),&search);
	 			if(search == ip_hdr->daddr){
	 				contor = 1;
	 				break;
	 			}
	 		}
	 		if(contor == 1){
	 			icmp_hdr->type = 0;
	 			init_hdrs(eth_hdr,ip_hdr,icmp_hdr,ip_hdr->saddr,ip_hdr->daddr,eth_hdr->ether_dhost,eth_hdr->ether_shost);
	 			send_packet(m.interface,&m);
	 			printf("Pachet trimis!\n");
	 			continue;
	 		}
		}

	 	ip_hdr->ttl--;
	 	ip_hdr->check = 0;
	 	ip_hdr->check = ip_checksum(ip_hdr,sizeof(struct iphdr));
	 	struct arp_entry *ar = get_arp(ip_hdr->daddr);
	 	if(ar == NULL){
	 		printf("No ARP entry found\n");
	 		continue;
	 	}
	 	memcpy(eth_hdr->ether_dhost,ar->mac,sizeof(ar->mac) + 1 );
	 	send_packet(rtabl->interface,&m);
	 }

}

