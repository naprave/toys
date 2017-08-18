#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <libmemcached/memcached.h>

struct sockaddr_in localSock;
struct sockaddr_in groupSock;
struct ip_mreq group;
int sd,sd2;
int datalen;
char databuf[32768];
char cameraloc[1000];
char cameraip[1000];
char camerausn[1000];

void perror(char * err) {
	printf("ERR: %s\n", err);
}

int main (int argc, char *argv[ ])
{
	/* Create a datagram socket on which to send. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
	perror("Opening datagram socket error");
	exit(1);
	}
	else
	printf("Opening the datagram socket...OK.\n");

	
	{
	int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
	{
	perror("Setting SO_REUSEADDR error");
	close(sd);
	exit(1);
	}
	else
	printf("Setting SO_REUSEADDR...OK.\n");
	}	
	
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(49154);
	localSock.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
	{
		perror("Binding datagram socket error i");
		close(sd);
		exit(1);
	} else {
		printf("Binding datagram socket...OK.\n");
	}
	
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	//group.imr_interface.s_addr = inet_addr("203.106.93.94");
	group.imr_interface.s_addr = inet_addr("0.0.0.0");
//Internet Protocol Version 4, Src: 192.168.1.104 (192.168.1.104), Dst: 239.255.255.250 (239.255.255.250)	
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
	perror("Adding multicast group error");
	close(sd);
	exit(1);
	}
	else
	printf("Adding multicast group...OK.\n");
	
	char* req = "M-SEARCH * HTTP/1.1\r\n\
HOST: 239.255.255.250:1900\r\n\
MAN: \"ssdp:discover\"\r\n\
MX: 3\r\n\
ST: urn:schemas-upnp-org:device:MediaServer:1\r\n\
\r\n\
";
	int reqlen = strlen(req);
	
/*
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("239.255.255.250");
	groupSock.sin_port = htons(4321);


	if(sendto(sd, req, reqlen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
	{perror("Sending datagram message error");}
	else
	printf("Sending datagram message...OK\n");
*/	


	int sockfd,n;
	struct sockaddr_in servaddr,cliaddr;
	char sendline[1000];
	char recvline[1000];


	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr("239.255.255.250");
	servaddr.sin_port=htons(1900);

	sendto(sockfd,req,strlen(req),0, (struct sockaddr *)&servaddr,sizeof(servaddr));
	//sleep(2);
	//sendto(sockfd,req,strlen(req),0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	datalen = sizeof(databuf);
	
/** read response, parse camera location, send http capabilites and startstream command */	

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		exit(69);
	}

	const char *config_string= "--SERVER=127.0.0.1";
	
	memcached_st *memc = memcached(config_string, strlen(config_string));
	
	char* pos = 0;

	for( int cam=0; cam<8; cam++ ) {
		if(int readbytes = read(sockfd, databuf, datalen) < 0)
		{
			perror("No response from cameras");
			//close(sd);
			//exit(1);
		} else {
			pos = strstr(databuf, "4D454930");
			if( pos == 0 ) continue;
//			printf(databuf);
			pos = strstr(databuf, "LOCATION:");
			if( pos != NULL ) {
				strcpy(cameraloc, pos+10);
				//printf(cameraloc);
				pos = strstr(cameraloc, "\r");
				if( pos != NULL ) {
					cameraloc[pos-cameraloc] = 0;
					//pos[0] = 0;
				} else {
					cameraloc[0] = 0;
					exit(2);
				}
			} else exit(2);

			pos = strstr(cameraloc, "//");
			strcpy(cameraip, pos+2);
			pos = strstr(cameraip, ":");
			if( pos != NULL ) {
				cameraip[pos-cameraip] = 0;
				//pos[0] = 0;
			} else {
				exit(2);
			}
			
			pos = strstr(databuf, "uuid:");
			if( pos == 0 ) continue;
			sprintf(camerausn, "camera_");
			strcat(camerausn, pos+29);
			camerausn[19] = 0;

			printf(cameraloc);
			printf(cameraip);
			printf(camerausn);
			//printf(cameraloc);
			//printf(cameraloc);

		    
			//char *key= "camera";
			char *key= camerausn;
			//char *value= "value";

			memcached_return_t rc = memcached_set(memc, key, strlen(key), cameraip, strlen(cameraip), (time_t)0, (uint32_t)0);

			if (rc != MEMCACHED_SUCCESS)
			{
			//... // handle failure
				printf("failed to write to memcache?!");
			}
			
			
		}

	}
/****/
	

	if(setsockopt(sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
	perror("Adding multicast group error");
	close(sd);
	exit(1);
	}
	else
	printf("Adding multicast group...OK.\n");

	
	
	memcached_free(memc);	

 exit(22);
	
	FILE* fp = fopen("out.mp4","w");
	
	int buffersize = 19999000;
	int written = 0;
	/* Read from the socket. */
	char packetfilename[255];
	
	int i = 0;
	while( written < buffersize ) {
		if(int readbytes = read(sd, databuf, datalen) < 0)
		{
			perror("Reading datagram message error f");
			close(sd);
			exit(1);
		} else {
			//readbytes = datalen;
			printf("Reading datagram message...OK %d.\n", readbytes);
		//printf("The message from multicast server is: \"%s\"\n", databuf);
			if(readbytes>=12) fwrite( databuf+12, readbytes-12, 1, fp);
			written += readbytes;
			
			sprintf(packetfilename, "packet%d.udp", i++ );
			FILE* fpacket = fopen("out.mp4","w");
			fwrite( databuf, readbytes, 1, fpacket);
			fclose(fpacket);
			
		}

	}
	fclose(fp);
	
	close(sd);
	sleep(1);


	return 0;
}