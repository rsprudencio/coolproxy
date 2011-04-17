/*************************************************************
 *
 * 	DCLABS 2011 - raph0x88
 *
 *
 *	Usage: ./coolproxy
 *
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
#include <curl/curl.h>
#include "coolproxy.h"



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



char **
regexFilter(char *string, size_t *sizeList, char *RegExp)
{
	size_t s = 0;
	char **list = NULL;
    regex_t re;
    regmatch_t result;
    int diff;



	/*
	*  USING REGEX TO DO THE FILTERING
	*/

	if(regcomp(&re, RegExp, REG_EXTENDED) != 0)
		return NULL;

	while(regexec(&re, string, 1, &result, 0) == 0)
	{
		string += result.rm_so;
		diff = result.rm_eo - result.rm_so;
		s++;
		list = realloc(list, s * sizeof(char *));
		list[s-1] = strndup(string, diff);
		list[s-1][diff] = '\0';
		string += result.rm_eo - result.rm_so;
	}

    regfree(&re);

	*sizeList = s;

	return list;
}



char **
testProxies(char **list, size_t sizeList, size_t *sizeCoolProxies, char *ownIp)
{
	char http[HTTPRESULT] = { 0 };
	char tempChar[25];
	char **proxy, **coolProxies = NULL;
	char *ip, *port;
	int i;
	size_t sizeProxies = 0;
	CURL *curl;
	CURLcode code;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, readCurl);
    curl_easy_setopt(curl, CURLOPT_URL, GETIPSITE);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);



	for(i = 0; i < sizeList; i++)
	{
		ip = strtok_r(list[i], ":", &port);

		curl_easy_setopt(curl, CURLOPT_PROXY, ip);
		curl_easy_setopt(curl, CURLOPT_PROXYPORT, atoi(port));

		printf("testing %s:%s\n", ip, port);

		if((code = curl_easy_perform(curl)) == 0)
		{
			proxy = regexFilter(http, &sizeProxies, IP_REGEXP);

			printf("Number of proxies found: %d\n", sizeProxies);

			if(proxy != NULL)
			{
				printf("\tShown %s\t comparing with %s - IF DIFF SHOULD APPEAR BELOW!\n", proxy[0], ownIp);
				if(strcmp(proxy[0], ownIp))
				{
					strcpy(tempChar, ip);
					strcat(tempChar, ":");
					strcat(tempChar, port);
					fprintf(stdout, "\t%s\n", tempChar);
					*sizeCoolProxies += 1;
					coolProxies = realloc(coolProxies, *sizeCoolProxies * sizeof(int));
					coolProxies[*sizeCoolProxies-1] = strdup(tempChar);
				}
			}

			memset(http, 0x00, strlen(http));
		}
		else
			printf("Code we got: %d\n", code);

		freeReallocd(proxy, sizeProxies);
	}

	curl_easy_cleanup(curl);

	return coolProxies;
}



void
getOwnIp(char ip[])
{
	char http[HTTPRESULT] = { 0 };
	char **result = NULL;
	size_t resultSize = 0;
	CURL *curl;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, GETOWNIPSITE);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, readCurl);
    curl_easy_perform(curl);

	result = regexFilter(http, &resultSize, IP_REGEXP);

	if(result != NULL)
		strncpy(ip, result[0], IPSIZE-1);
	else
	{
		fprintf(stderr, "failed to get own external ip\n");
		exit(1);
	}

	freeReallocd(result, resultSize);
}



void
freeReallocd(char **buffer, size_t sizeBuffer)
{
	int i;


	if(buffer != NULL)
	{
		for(i = 0; i < sizeBuffer; i++)
		{
			if(buffer[i] != NULL)
				free(buffer[i]);
		}

		//free(buffer);
	}
}



void
readIni(size_t *sizeSources, char **sourceProxies){
	FILE *fp;
	char temp[1024];
	char *token, *tmp1, *token2;
	int i, s = *sizeSources;



	fp = fopen("coolproxy.ini", "r");
	if(fp == NULL)
		return;



	/*
	 * SCANNING THE INI FILE
	 */

	//while(fscanf(fp, "%s", temp) > 0)
	while(fgets(temp, 1023, fp) != NULL)
	{
		temp[strlen(temp)-1] = '\0';

		if(strchr(temp, '#') != NULL)
			continue;

		for (i = 1, tmp1 = temp; ; i++, tmp1 = NULL)
		{
			token = strtok_r(tmp1, "=", &token2);
			if (token == NULL)
				break;

			if(!strcmp(token, "SOURCE"))
			{
				s++;
				sourceProxies = realloc(sourceProxies, sizeof(int) * s);
				sourceProxies[s-1] = strdup(token2);
			}
		}
	}


	fclose(fp);

	*sizeSources = s;
}



size_t
readCurl(void *curlData, size_t nmemb, size_t size, void *http)
{
	strncat(http, curlData, HTTPRESULT - strlen(http));

	return size * nmemb;
}


int
main(int argc, char **argv)
{
    char http[HTTPRESULT] = { 0 };
    char **list = NULL, **coolProxies = NULL, **sourceProxies = NULL;
    char ownIp[IPSIZE];
    size_t sizeSources = 0,sizeCoolProxies = 0, sizeList = 0;
    int i = 0;
    CURL *curl;



	if(argc != 1)
		usage(argv);

	fprintf(stdout, "\nDcLabs 2010 - CoolProxy %s", VERSION);
	fprintf(stdout, "\n  Retrieving and testing proxies...\n");



	/*
	 *  READING CONFIGURATION FILE
	 */

	if(sourceProxies == NULL)
	{
		sourceProxies = malloc(sizeof(int));
		sourceProxies[0] = strdup(DEFAULTSITE);
		sizeSources++;
	}

	readIni(&sizeSources, sourceProxies);



    /*
     *  GETTING OWN IP
     */

    getOwnIp(ownIp);



    /*
     *  LOOP THAT GET AND TEST PROXIES
     */

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, readCurl);

    for(i = 0; i < sizeSources; i++)
    {


		/*
		 *  GETTING PROXIES
		 */


    	curl_easy_setopt(curl, CURLOPT_URL, sourceProxies[i]);
        curl_easy_perform(curl);

		if(http == NULL)
			continue;



		/*
		 *  FILTERING THE IP's FROM HTTP RESPONSE
		 */

		list = regexFilter(http, &sizeList, IPPORT_REGEXP);



		/*
		 *  TESTING PROXIES
		 */

		coolProxies = testProxies(list, sizeList, &sizeCoolProxies, ownIp);



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
    
    curl_easy_cleanup(curl);

    return 0;
}
