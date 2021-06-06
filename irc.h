#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdarg.h>
//#include<regex.h>
//#define URL_REGEX "https?:\\/\\/(www\\.)?[-a-zA-Z0-9@:%._\\+~#=]{1,256}\\.[a-zA-Z0-9()]{1,6}\\b([-a-zA-Z0-9()@:%_\\+.~#?&//=]*)"

#define FAIL    -1
#define BUF_SIZE 1024
#ifndef _bool
#define _bool 
typedef enum{false,true}bool;
#endif

static volatile bool ssl_inited=false;
/*static regex_t regex_url;
static volatile bool regex_inited =false;*/

struct IRCConnection{
	int socket;
	SSL * ssl_socket;
	SSL_CTX * ctx;
	bool isSSLConnection;
	char * host; int port;
};
struct IRCConnection  OpenConnection(const char *hostname, int port, bool useSSL);
SSL_CTX* InitCTX(void);
void ShowCerts(SSL* ssl);
void freeConnect(struct IRCConnection *);
typedef struct IRCConnection ircc;

void irc_sendMsg(ircc c, const char * msg);
size_t irc_recvMsg(ircc c, char * buf, size_t n);

void regOnServ(ircc c, const char * nick, const char * username, const char * realname);

void joinChn(ircc c, const char * channel, ...);

void recvHandler(ircc c);

void pingIfNeed(ircc c, const char * last_buf);

void msg_handler(ircc c, const char ** splitted, const size_t splitted_size);

char ** split_msg(char * buf, const char schar, size_t * splitted_size);
void free_splitted(char  ** what, size_t n);

void PRIVMSG(ircc c, const char * chnusr, const char * msg, const unsigned char color);

