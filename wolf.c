/*
 ============================================================================
 Name        : wolf.c
 Author      : Depicus
 Version     : 1.1
 Copyright   : One of the free ones, not sure - who cares
 Description : Wake on Lan Forwarder or WolF for short allows you to redirect WoL packets over the internet.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#ifdef __APPLE__
//define something for os x
#define ETHNAME "en0"
#define PPPNAME "en0"
#else
//define it for anything else
#define ETHNAME "eth0"
#define PPPNAME "ppp0"
#endif

char ppp0a[16];
char str[1024];
static const int port = 4343;
static const int outport = 4567;

char const *getadapteraddress(char adapter[5])
{
    struct sockaddr_in sin;
    in_addr_t subnet;
    
    int fd;
    struct ifreq ifr;
    u_char *addr;
    fd = socket (AF_INET, SOCK_DGRAM,0);
    if (fd < 0)
    {
        fprintf(stderr,"Error: Unable to create socket\n");
        perror("socket");
        return "Error";
    }
    memset (&ifr, 0, sizeof (struct ifreq));
    strcpy (ifr.ifr_name, adapter);
    ifr.ifr_addr.sa_family = AF_INET;
    ioctl(fd, SIOCGIFADDR, &ifr);
    addr=(u_char*)&(((struct sockaddr_in * )&ifr.ifr_addr)->sin_addr);
    //printf("eth %s, addr %d.%d.%d.%d\n", ifr.ifr_name,addr[0],addr[1],addr[2],addr[3]);
    sprintf (ppp0a,"%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
    
    // get the subnet mask
    memset(&sin, 0, sizeof(struct sockaddr));
    memset(&ifr, 0, sizeof(ifr));
    strcpy (ifr.ifr_name, adapter);
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFNETMASK, &ifr)< 0)    {
        perror("ioctl() - get subnet");
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr));
    subnet = sin.sin_addr.s_addr;
    close(fd);
    
    char ipstr[INET_ADDRSTRLEN];
    // now get it back and print it
    inet_ntop(AF_INET, &(subnet), ipstr, INET_ADDRSTRLEN);
    printf("subnet %s \n",ipstr);
    
    return ppp0a;
}

int main(void) {
	printf("Starting WoLf (Wake On Lan Forwarder) \n\n");
	char const *cptr;
	cptr = getadapteraddress(PPPNAME);
	printf("ppp0 = %s\n",cptr);
	cptr = getadapteraddress(ETHNAME);
	printf("eth0 = %s\n\n",cptr);
    
    
    int sendSocket;
    long inPacketSize;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t len;
    unsigned char mesg[101]; //wol packet size is 102 so 0 to 101 - what about secure password ?
    
    sendSocket=socket(AF_INET,SOCK_DGRAM,0);
    
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);
    bind(sendSocket,(struct sockaddr *)&servaddr,sizeof(servaddr));
    
    for (;;)
    {
        printf("Getting a packet\n");
        printf("waiting on port %d\n", port);
        
        len = sizeof(cliaddr);
        inPacketSize = recvfrom(sendSocket,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
        
        
        if ((inPacketSize != 102) && (inPacketSize != 108))
        {
            printf("packet was wrong size we got %ld \n", inPacketSize);
            continue;
        }
        
        
        printf("we got %ld\n",inPacketSize);
        struct sockaddr_in wt;
        memset(&wt, 0, sizeof(wt));
        wt.sin_family = AF_INET;
        wt.sin_port = htons(outport);
        wt.sin_addr.s_addr = inet_addr("10.11.11.255");

        int udpflag = 1;
        int retval;
        retval = setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, &udpflag, sizeof(udpflag));
        if (retval < 0)
        {
            sprintf (str,"failed to setsockopt: %s",strerror(errno));
            printf("%s\n",str);
        }
        
        long res;
        res = sendto(sendSocket,mesg,inPacketSize,0,(struct sockaddr *)&wt,sizeof(wt));
        
        if (res < 0)
        {
            sprintf (str,"failed to send: %s",strerror(errno));
            printf("%s\n", str);
        }
        
        printf("\nPacket Received\n");
        mesg[inPacketSize] = 0;
        int y;
        for (y = 0; y < sizeof(mesg) + 1; y++)
        {
            //printf("%i  %02x \n",y,mesg[y]);
        }
        //printf("%s",mesg);
        printf("\nEnd Packet\n");
    }
    printf("We should never ever get here \n");
}

