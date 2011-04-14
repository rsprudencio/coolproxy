/*************************************************************
 *
 * 	DCLABS 2010 - raph0x88
 *
 *	CoolProxy v0.6
 *
 *
 *	Usage: ./coolproxy
 *
 *
 *
 *	Its an alpha-version of the tool so it has some bugs
 *
 *	Please report any bug/problem at dmind.org/dclabs
 *
 *************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <regex.h>


#define VERSION			"v0.6"
#define TIMEOUT_RETRIES		3
#define RESULTBUFSIZE		65536
#define TEMPBUFSIZE		3000
#define HTTPHEADERSIZE		2048
#define IPPORT_REGEXP		"[0-9]{1,3}([.][0-9]{1,3}){3}:[0-9]{1,5}" 	// "([1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(.([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}:([1-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-9][0-9][0-9][0-9]|[1-6][0-5][0-5][0-3][0-5])"
#define IP_REGEXP			"[0-9]{1,3}([.][0-9]{1,3}){3}"
#define DEFAULTSITE		"www.proxy-list.net"
#define DEFAULTPAGE    		"/anonymous-proxy-lists.shtml"
#define GETIPSITE			"meuip.datahouse.com.br" 							// /whatismyipaddress.com



/*
 *  PROTOTYPES
 */

void freeReallocd(char **, int);
void getOwnIp();
char **testProxies(char **, int, int *);
char **regexFilter(char *, int *, char *);
char *getHttp(char *, char *, char *);



/*
 *  GLOBAL VARIABLES
 */

char *ownIp;
char **sourceProxies;
char **sourcePage;
struct timeval sendTimeout;
struct timeval recvTimeout;



/*
 *  FUNCTIONS
 */

void
usage(char **argv)
{
	fprintf(stdout, "Usage: %s \n", argv[0]);
	fprintf(stdout, "	By DcLabs - www.dmind.org/dclabs\n");
	exit(1);
}



/*
 * Function getHttp
 *
 * 		description: returns a buffer with the
 * 					 raw HTTP response from the server
 *
 *		usage: char *ptr = getHttp("www.google.com", "80", "/search?=aa");
 */

char *
getHttp(char *proxySite, char *proxySitePort, char *proxyPage)
{
	struct addrinfo hints, *res, *p;
	int status, retry = 0;
	int sockfd;
	int yes=1, numbytes;
	char buffer[RESULTBUFSIZE] = "\0";
	char tempbuf[TEMPBUFSIZE];



	/*
	 *  GETTING INFO FOR SOCKET
	 */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(proxySite, proxySitePort, &hints, &res)) != 0)
		return NULL;



	/*
	 *  TRYING TO GET A SOCKET FROM THE INFO
	 */



	for(p = res; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
			continue;

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout,  sizeof recvTimeout))
			continue;

		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendTimeout,  sizeof sendTimeout))
			continue;

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
			continue;

		break;
    }

	if(p == NULL)
		return NULL;



    /*
     *  SETTING HTTP HEADER
     */

    char * headerHttp = (char *) malloc(HTTPHEADERSIZE * sizeof(char));
    
    if (headerHttp == NULL) 
        return NULL;
    
    strcpy(headerHttp, "GET ");
    strcat(headerHttp, proxyPage);
    strcat(headerHttp, " HTTP/1.1\n");
    strcat(headerHttp, "Host: ");
    strcat(headerHttp, proxySite);
    strcat(headerHttp, "\n");
    strcat(headerHttp, "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.0.4) Gecko/2008102920 Firefox/3.0.4\n");
    strcat(headerHttp, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\n");
    strcat(headerHttp, "Accept-Language: en-us,en;q=0.5\n");
    strcat(headerHttp, "Accept-Encoding: gzip,deflate\n");
    strcat(headerHttp, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\n");
    strcat(headerHttp, "Keep-Alive: 300\n");
    strcat(headerHttp, "Connection: keep-alive\n");
    strcat(headerHttp, "\x0d\x0a\x0d\x0a");
    headerHttp[strlen(headerHttp)] = 0;


    /*
     *  SENDING HTTP REQUEST
     */

	if (send(sockfd, headerHttp, HTTPHEADERSIZE, 0) == -1)
                return NULL;



    /*
     *  RECEIVING PACKETS AND PARSING
     */

	for(;;)
	{
		if((numbytes = recv(sockfd, tempbuf, TEMPBUFSIZE-1, 0)) == -1 || retry > TIMEOUT_RETRIES)
		{
			if(numbytes == -1)
				break;
			else
			{
				retry++;
				continue;
			}
		}

		if(numbytes == 0)
			break;

		strcat(buffer, tempbuf);
	}


    if (headerHttp != NULL)
        free(headerHttp);

	freeaddrinfo(res);
	return strdup(buffer);
}



char **
regexFilter(char *string, int *sizeList, char *RegExp)
{
	int s = *sizeList;
	char **list = NULL;
    regex_t re;
    regmatch_t result;



	/*
	*  USING REGEX TO DO THE FILTERING
	*/

	if(regcomp(&re, RegExp, REG_EXTENDED) != 0)
		return NULL;

	while(regexec(&re, string, 1, &result, 0) == 0)
	{
		string += result.rm_so;

		s++;
		list = realloc(list, s * sizeof(int));
		list[s-1] = strndup(string, result.rm_eo - result.rm_so);

		string += result.rm_eo - result.rm_so;
	}

    regfree(&re);

	*sizeList = s;

	return list;
}



char **
testProxies(char **list, int sizeList, int *sizeCoolProxies)
{
	char *http = NULL;
	char **proxy, **coolProxies = NULL;
	char *ip, *port;
	int i;
	int sizeProxies = 0;



	for(i = 0; i < sizeList; i++)
	{
		ip = strtok_r(list[i], ":", &port);

		if((http = getHttp(ip, port, "http://meuip.datahouse.com.br/")) != NULL)
		{
			proxy = regexFilter(http, &sizeProxies, IP_REGEXP);
			if(proxy != NULL)
			{
				if(strcmp(proxy[0], ownIp))
				{
					char tempChar[25];
					strcpy(tempChar, ip);
					strcat(tempChar, ":");
					strcat(tempChar, port);
					fprintf(stdout, "\t%s\n", tempChar);
					*sizeCoolProxies += 1;
					coolProxies = realloc(coolProxies, *sizeCoolProxies * sizeof(int));
					coolProxies[*sizeCoolProxies-1] = strdup(tempChar);
				}
			}
		}

		sizeProxies = 0;
	}



	return coolProxies;
}



void
getOwnIp()
{
	char *http;
	char **result = NULL;
	int resultSize = 0;



	http = getHttp(GETIPSITE, "80", " / ");

	result = regexFilter(http, &resultSize, IP_REGEXP);

	if(result != NULL)
		ownIp = strdup(result[0]);
	else
	{
		fprintf(stderr, "failed to get own external ip\n");
		exit(1);
	}



	freeReallocd(result, resultSize);
	if(http != NULL)
		free(http);
}



void
freeReallocd(char **buffer, int sizeBuffer)
{
	int i;

	for(i = 0; i < sizeBuffer; i++)
	{
		if(buffer[i] != NULL)
			free(buffer[i]);
	}
	if(buffer != NULL)
		free(buffer);
}



void
readIni(int *sizeSources){
	FILE *fp;
	char temp[1024];
	char *token, *tmp1, *token2;
	int i, s = *sizeSources;



	fp = fopen("coolproxy.ini", "r");
	if(fp == NULL)
		return;



	/*
	 *  INITIALIZING TIMEOUTS
	 */

	sendTimeout.tv_sec = 10;
	sendTimeout.tv_usec = 0;
	recvTimeout.tv_sec = 5;
	recvTimeout.tv_usec = 0;



	/*
	 * SCANNING THE INI FILE
	 */

	while(fscanf(fp, "%s", temp) > 0)
	{
		for (i = 1, tmp1 = temp; ; i++, tmp1 = NULL)
		{
			token = strtok_r(tmp1, "=", &token2);
			if (token == NULL)
				break;

			if(!strcmp(token, "SOURCE"))
			{
				char *tempToken;
				if( strncmp(token2, "http://", 7) == 0 )	/* REMOVES THE "http://" FROM THE LINK */
					token2 += 7;

				token = strtok_r(token2, "/", &token2);
				tempToken = malloc(strlen(token2) + 2);
				strcpy(tempToken, "/");
				strcat(tempToken, token2);

				s++;
				sourceProxies = realloc(sourceProxies, sizeof(int) * s);
				sourceProxies[s-1] = strdup(token);

				sourcePage = realloc(sourcePage, sizeof(int) * s);
				sourcePage[s-1] = strdup(tempToken);

				if(tempToken != NULL)
					free(tempToken);
			}

			else if(!strcmp(token, "SEND_TIMEOUT"))
				sendTimeout.tv_sec = atoi(token2);
			else if(!strcmp(token, "RECV_TIMEOUT"))
				recvTimeout.tv_sec = atoi(token2);
		}
	}

	fclose(fp);

	*sizeSources = s;
}



int
main(int argc, char **argv)
{

    char *http = NULL;
    int i = 0;
    char **list = NULL; int sizeList = 0;
    char **coolProxies = NULL; int sizeCoolProxies = 0;
    int sizeSources = 0;



	if(argc != 1)
		usage(argv);

	fprintf(stdout, "\nDcLabs 2010 - CoolProxy %s", VERSION);
	fprintf(stdout, "\n  Retrieving and testing proxies...\n");

	/*
	 *  READING CONFIGURATION FILE
	 */

	readIni(&sizeSources);

	if(sourceProxies == NULL || sourcePage == NULL)
	{
		sourceProxies = malloc(sizeof(int));
		sourceProxies[0] = strdup(DEFAULTSITE);
		sourcePage = malloc(sizeof(int));
		sourcePage[0] = strdup(DEFAULTPAGE);

		sizeSources++;
	}



    /*
     *  GETTING OWN IP
     */

    getOwnIp();



    /*
     *  LOOP THAT GET AND TEST PROXIES
     */

    for(i = 0; i < sizeSources; i++)
    {


		/*
		 *  GETTING PROXIES
		 */

		http = getHttp(sourceProxies[i], "80", sourcePage[i]);

		if(http == NULL)
			continue;



		/*
		 *  FILTERING THE IP's FROM HTTP RESPONSE
		 */

		list = regexFilter(http, &sizeList, IPPORT_REGEXP);



		/*
		 *  TESTING PROXIES
		 */

		coolProxies = testProxies(list, sizeList, &sizeCoolProxies);



		/*
		 *  RESET TO NEXT ROUND
		 */

	    freeReallocd(list, sizeList);
	    freeReallocd(coolProxies, sizeCoolProxies);
		sizeCoolProxies = 0;
		sizeList = 0;
    }



    /*
     *  FREEING MEMORY ALLOC'ED BY regexFilter()
     */

    freeReallocd(sourceProxies, sizeSources == 0 ? 1 : sizeSources);



    return 0;
}
