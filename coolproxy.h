#define VERSION                 "v0.7.2"
#define TIMEOUT_RETRIES         3
#define HTTPRESULT				65536
#define TEMPBUFSIZE             3000
#define HTTPHEADERSIZE          2048
#define IPPORT_REGEXP           "[0-9]{1,3}([.][0-9]{1,3}){3}:[0-9]{1,5}"
#define IP_REGEXP				"[[:digit:]]{1,3}(\\.[[:digit:]]{1,3}){3}"
#define DEFAULTSITE             "www.proxy-list.net/anonymous-proxy-lists.shtml"
#define GETIPSITE               "meuip.datahouse.com.br"          // /whatismyipaddress.com
#define GETOWNIPSITE			"http://checkip.dyndns.org/"
#define IPSIZE					21



/*
 *  PROTOTYPES
 */

void freeReallocd(char **, size_t);
void getOwnIp(char []);
char **testProxies(char **, size_t, size_t *, char *);
char **regexFilter(char *, size_t *, char *);
void readIni(size_t *, char **);
void usage(char **);
size_t readCurl(void *, size_t, size_t, void *);
