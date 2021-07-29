#include"irc.h"
#include"curl.h"
#include <fcntl.h>
static time_t lastPage;
static const char admin_nick[] = ADMIN_NICK;
static const char * permamentbanned[]={
	"localhost",
	"cofob",
	"owo"
};static size_t permamentbanned_s = 3;

static char ** banned;
static size_t bannedS = 0;

static bool userNotBanned(const char * nick){
	for(unsigned int i =0; i < bannedS && banned[i] != NULL; i++){
		if( strcasecmp(banned[i], nick) == 0){
			return false;
		}
	}
	for(unsigned int i = 0; i < permamentbanned_s; i++){
		if( strcasecmp(permamentbanned[i], nick) == 0){
			return false;
		}
	}
	return true;
}

static void ban_user(const char * nick){
	if( !userNotBanned(nick) ) return;
	if( strcasecmp(admin_nick, nick) == 0 ) return;
	bannedS++;
	if(banned == NULL) banned=malloc(sizeof(char*));
	else
		banned = realloc(banned,sizeof(char*) * bannedS);
	banned[bannedS-1] = malloc(sizeof(char) * strlen(nick) + 1);
	strcpy(banned[bannedS-1], nick);	
}


SSL_CTX* 
InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void 
ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

struct IRCConnection 
OpenConnection(const char *hostname, int port, bool useSSL)
{
    struct IRCConnection ret;
    ret.isSSLConnection = false;
    if(useSSL){
	if(!ssl_inited){
		SSL_library_init();
		ssl_inited=true;
	}
    ret.isSSLConnection = true;
    }


    struct hostent *host;
    struct sockaddr_in addr;
    if ( (host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    ret.socket = socket(PF_INET, SOCK_STREAM, 0);

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    struct timeval timeout;
    timeout.tv_sec=120;
    timeout.tv_usec=0;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(ret.socket, &rfds);
    if( select(ret.socket+1,&rfds,NULL,NULL,&timeout) == -1 ){
		perror("select() timeout");
    }

    //int oldflags =fcntl(ret.socket, F_GETFL, NULL);
    //fcntl(ret.socket, F_SETFL, oldflags|O_NONBLOCK);
    if ( connect(ret.socket, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(ret.socket);
        perror(hostname);
        //abort();
    }
    ret.socket=ret.socket;
    if(useSSL){
	    ret.ctx = InitCTX();
    	    ret.ssl_socket = SSL_new(ret.ctx);      /* create new SSL connection state */
    	    SSL_set_fd(ret.ssl_socket, ret.socket);    /* attach the socket descriptor */
   	    if ( SSL_connect(ret.ssl_socket) == FAIL ){   /* perform the connection */
        		ERR_print_errors_fp(stderr);
			perror(hostname);

			freeConnect(&ret);
			struct IRCConnection noinfo;;
			return noinfo;
	    }
	    ShowCerts(ret.ssl_socket);        /* get any certs */
	    ret.isSSLConnection=true;
    }else  ret.isSSLConnection=false;
    ret.host=malloc( strlen(hostname)+1 * sizeof(char) );
    memcpy(ret.host, hostname, strlen(hostname));
    ret.host[strlen(hostname)]='\0';
    ret.port = port;
    return ret;
}

void irc_sendMsg(ircc c, const char * msg){
	if(c.isSSLConnection)
	  SSL_write(c.ssl_socket,msg, strlen(msg));
	else
	 write(c.socket, msg, strlen(msg));
}
size_t irc_recvMsg(ircc c, char * buf, size_t n){
	if(c.isSSLConnection){
		return SSL_read(c.ssl_socket, buf, n); 
	}
	//puts("Read");
	return read(c.socket, buf, n);
}

void 
pingIfNeed(ircc c, const char * last_buf){
		char * pPing;
		if( (pPing=strstr(last_buf, "PING :")) != 0 ){

			char buf[BUF_SIZE];
			//bzero(buf,sizeof(BUF_SIZE));
			size_t bufc=0;
			void *sBuf = buf;
			//size_t size_pong;
			//puts("PING - PONG");
			//PING :ZASDFDFSDF\n
			pPing += 6;
			//printf("%s\n",pPing);
			//char * tmp = pPing;
			while(*(pPing) != '\n' && *(pPing) != '\0'){
				buf[bufc++]=*(pPing++);
			}
			buf[bufc++]='\0';
			if(*pPing !='\n') return;

			printf("buf: %s\n",buf);
			char pongLine[sizeof("PONG :")+bufc+sizeof("\r\n")+1];
			sprintf(pongLine, "PONG :%s\r\n", buf);
			printf("buf: %s\n",pongLine);
			irc_sendMsg(c, pongLine);
		}//if found PING 
}

void 
regOnServ(ircc c, const char * nick, const char * username, const char * realname){
	char buf[BUF_SIZE];
	sprintf(buf, "NICK %s\r\nUSER %s * * %s\r\n", nick, username, realname);
	irc_sendMsg(c, buf);
	if (irc_recvMsg(c,buf,BUF_SIZE)){
		pingIfNeed(c, buf);
		puts("Connected to server");
	}// if recvMsg
	//puts(buf);
}

void 
joinChn(ircc c, const char * channel, ...){
	va_list ap;
	char buf[BUF_SIZE];
	va_start(ap, channel);
	while(*channel && *channel =='#'){
		sprintf(buf,"JOIN %s\r\n",channel);
		irc_sendMsg(c,buf);
		printf("JOIN TO %s\n", channel);
		channel = va_arg(ap, char*);
	}
	va_end(ap);


}

char ** 
split_msg(char * buf, const char schar, size_t * splitted_size){
/*
*/
	const unsigned long long max_splitted = 120;
	size_t arr_size=0;
	char **splitted;
	splitted = (char**)malloc( sizeof(char*) * arr_size );
	if(!splitted) abort();
	char * str;
	char str2[BUF_SIZE];
	do{
		bzero(str2, BUF_SIZE);
		str = strchr(buf, schar);
	

		if(str != NULL || ( str = strchr(buf, '\r') )!=NULL ){
			arr_size++;
			splitted = (char**)realloc( splitted,sizeof(char*) * arr_size );
			if(!splitted) abort();

			memcpy(str2, buf, (str-buf));
			//printf("str2 = %s\n", str2);

			splitted[arr_size-1] = (char*)malloc( sizeof(char) * strlen(str2) + 1);
			if( !splitted[arr_size-1] ) abort();
			
			strcpy(splitted[arr_size-1], str2); 
			splitted[arr_size-1][strlen(str2)]=0;
		}
		buf=str+1;
	}while(str != NULL && arr_size < max_splitted);
	*splitted_size=arr_size;
	return splitted;
}
void 
free_splitted(char  ** what, size_t n){
	for(n;n--;){
		if(n == 0) break;
		if( what[n][0] == 0 ) continue;
		free((void*) (what[n-1]) );
	}
	free((void*)what);
}

void 
PRIVMSG(ircc c, const char * chnusr, const char * msg, const unsigned short color){
	char buf[BUF_SIZE];
	bzero(buf, BUF_SIZE);
	#define COLOR_CHAR ""
	char color_buf[4];
	bzero(color_buf, sizeof(color_buf));
	if(color != 0)
		sprintf(color_buf, COLOR_CHAR"%d", color);
	else
		sprintf(color_buf, COLOR_CHAR"");
	
	sprintf(buf,"PRIVMSG %s :%s%s\r\n",chnusr, color_buf, msg);
	printf("buf: %s\n", buf);
	irc_sendMsg(c,buf);
}
unsigned long long  _strlen_without_space(void * buf){
	unsigned long long s=0;
	char * b=buf;
	while(*(b++)){
		if(*b == ' ') continue;
		s++;
	}
	return s;
}
void 
welcome_handler(ircc c, const char ** splitted, size_t splitted_size, bool isJoin){
const char * phrases[]={
	"Ваш памятник будет построен в течение 900 дней",
	"И его лик осенил всех вокруг",
	"Всем по слогам его ник прочесть!",
	"Здравствуйте, Ваше светлоличие",
	"Здравствуйте, Здравствуйте. Туркменистан призывает хлопать Вам 5 минут в ладоши",
	"^-^"
};const size_t count_phrases = 6;
const char * phrases_bye[]={
	"Это всё из-за тёмных!",
	"Всем грустить пять минут!",
	"Очень жаль...",
	"Кто его обидел, а?!",
	"Кто ему не похлопал щас, а?"
};const size_t count_phrases_bye = 5;
	srand(time(NULL));
	const char *sender, *channel;
	sender=splitted[0];
//	sender=sender+1;
	channel=splitted[2];
//	channel=channel+1;
	size_t sChannel = strlen(channel);
	char channel_[sChannel+1];
	if(isJoin)
		strcpy(channel_,channel+1);
	else
		strcpy(channel_, "#ru");
	if( strstr(sender,"k60") == NULL ) return;

	for(unsigned int i = sChannel-1;i--;){
		if(i == sChannel-1) channel_[i] = '\0';
		if( channel_[i]== '\n' || channel_[i] == '\r' ) {
			channel_[i]='\0';
			break;
		}
	}
	
	char nickSender[BUF_SIZE];
	char buf[BUF_SIZE];
	bzero(nickSender, sizeof(nickSender));
	memcpy(nickSender, (sender+1), (strstr(sender,"!") -(sender+1)));
	if(isJoin)
		sprintf(buf,"%s Вошёл! %s\n", nickSender, phrases[rand()%count_phrases]);
	else
		sprintf(buf,"%s Ушёл! %s\n", nickSender, phrases_bye[rand()%count_phrases_bye]);
	printf("Sender: %s\n channel:%s\n",nickSender,channel_);
	printf("buf:%s \n",buf);
	PRIVMSG(c, channel_, buf, 2);
}

void 
msg_handler(ircc c, const char ** splitted, size_t splitted_size){
	/*if(!regex_inited){
		if(regcomp(&regex_url, URL_REGEX, 0) != 0) abort();
	}*/
	if(splitted_size < 3) return;
	const char *sender, *channel;
	sender=splitted[0];
	if(strstr(sender,"!") == NULL) return;
	channel=splitted[2];
	splitted[3] = splitted[3]+1;

	char nickSender[BUF_SIZE];
	bzero(nickSender, sizeof(nickSender));
	memcpy(nickSender, (sender+1), (strstr(sender,"!") -(sender+1)));

	//printf("%s send msg in %s -> ", sender, channel);
	#define UNALLOWED {return;}
	size_t i = 0;
	for(i=0; i < splitted_size;i++){
		for(unsigned un = 0; un < UNALLOWED_DOMAIN_COUNT;un++){
			//printf("i: %d;un:%d\n",i,un);
			if( strstr(splitted[i], unallowed_hosts[un]) != NULL){
				splitted[3] = splitted[3] - 1;
			       	UNALLOWED;
			}
		}
	}
	i = 3;
	char buf[BUF_SIZE];
	if(userNotBanned(nickSender))
	while( i < splitted_size  ){
		if( strcasecmp(nickSender, admin_nick) == 0 ){
			//is admin;
			puts("Is admin!");
			if( strstr(splitted[i], "!ban") != NULL && i+1 <= splitted_size && i == 3){
				puts("Ban user");
				if( userNotBanned( splitted[i+1] ) == false ){
					sprintf(buf, "%s забанен так то уже\n", splitted[i+1]);
				}else{
					ban_user(splitted[i+1]);
					sprintf(buf, "%s забанен нахуй\n", splitted[i+1]);
				}
				puts("User banned");
				PRIVMSG(c, channel, buf, 5);
				break;
			}else puts("Not found command.");
		}
		//printf("%s %d\n", splitted[i], i);
		//if( regexec(&regex_url, splitted[i], 0, NULL, 0) == 0 ){
	//	printf("splitted_size: %d\n", splitted_size);
		if( splitted[i] == 0 || splitted[i][0] == 0) {
			printf("Warning splitted[%d] = 0!\n", i);
			i++;
			continue;
		}
		if(strstr(splitted[i], "http://") != NULL || strstr(splitted[i], "https://") != NULL ){
		//	if( strstr(splitted[i], "127.0.0.1") != NULL) UNALLOWED;
		//	if( strstr(splitted[i], "192.168.") != NULL) UNALLOWED;
			//printf("Url: %s\n", splitted[i]);
			if( (time(NULL) - lastPage) < 45) { i++; continue; }
			lastPage=time(NULL);
			char about_page[BUF_SIZE];
			char buf[BUF_SIZE];
			bzero(about_page,sizeof(about_page));
			get_info_about_url(splitted[i], about_page);
			if(about_page[0] == 0){
				i++;
			       	continue;
			}
			printf("About_Page: %s\n", about_page);
			PRIVMSG(c, channel, about_page, 3);
		}else if( (strstr(splitted[i], "!ac") != NULL) && i == 3){
			i++;
			bzero(buf, sizeof(buf));
			while(i < splitted_size){
				sprintf(buf, "%s %s", buf,splitted[i++]);
			}
			printf("%s\n", buf);
			if(_strlen_without_space(buf) < 15) {
				sprintf(buf, "%s, КоРоТкАя цитата какая-то, 15 символов над:(: !ac <цитата>", nickSender);
				PRIVMSG(c, channel, buf, 4);
			}
			else if(addQuote(channel, nickSender, buf)){
				sprintf(buf, "%s, цитата добавлена,у неё должен быть номер %d\n", nickSender, getQuotesLength());
				PRIVMSG(c, channel, buf, 0);
			}

		}else if( (strstr(splitted[i], "!c") != NULL) && i == 3){
			char buf[BUF_SIZE];
			i++;
			long long quote_count=getQuotesLength();
			long long quote_id= ((i) < splitted_size) ? atoll(splitted[i]) : (rand() % quote_count)+1;
			printf("!c %ld %ld\n", quote_count, quote_id);
			if(quote_id == 0 || quote_id > quote_count || quote_count < 0){
				sprintf(buf, "%s, номер цитаты дан не правильным должен быть от 1-%d",nickSender, quote_count);
				PRIVMSG(c, channel, buf, 0);
				break;
			}

			puts("Цитата найдена");
			char cID[100];
			sprintf(cID,"%ld",quote_id);
			getQuote(cID, buf);
			PRIVMSG(c, channel, buf, 0);
			break;			
		}else if( (strstr(splitted[i], "!kubic") != NULL) && i == 3){
			sprintf(buf, "%s, Кубик выдал число: %d", nickSender, (rand() % 7)+1);
			PRIVMSG(c, channel, buf, 2);
			break;
		}else if( (strstr(splitted[i], "!source") != NULL) && i == 3){
			sprintf(buf, "%s", "https://github.com/wipedlifepotato/RusNetBot");
			PRIVMSG(c, channel, buf, 0);
			break;
		}else if( (strstr(splitted[i], "!help") != NULL) && i == 3){
			sprintf(buf, "%s", "!ac <quote> !c <num-quote> !kubic !source ...");
			PRIVMSG(c, channel, buf, 0);
			break;
		}


		i++;
	}
	#undef UNALLOWED
	splitted[3] = splitted[3]-1;
	puts("");
}

void 
recvHandler(ircc c){
    srand(time(NULL));
    char buf[BUF_SIZE];
    size_t rcvBytes;
    do{
	bzero(buf, BUF_SIZE);
	rcvBytes=irc_recvMsg(c, buf, BUF_SIZE);
	if(buf[0] == 0){
		break;
	}
	if(buf[0] == 0) break;
	printf("Recv: %s\n", buf);
	pingIfNeed(c,buf);

	size_t size;
	char ** splitted = split_msg(buf, ' ', &size);
	//puts("Splitted:");
	for(unsigned int i =0; i<size;i++){
		//strcasecmp ignore case
		if(strcmp(splitted[i], "PRIVMSG") == 0){
			msg_handler(c, (const char**)splitted, size);
		}else if( i == 1 && (strcmp(splitted[i], "JOIN") == 0 || strcmp(splitted[i],"PART") == 0
				       	|| strcmp(splitted[i], "QUIT") == 0 ) ){
			bool isJoin=false;
			if( splitted[i][0]=='J' ) isJoin=true;
			puts("Welcome Handler");
			welcome_handler(c, (const char**)splitted, size, isJoin);
		}
		else if( strcmp(splitted[i], "ERROR") == 0 && i == 0) break;
		//puts(splitted[i]);
	}
	free_splitted(splitted,size);

    }while(rcvBytes);

}

void 
freeConnect(struct IRCConnection * con){
    if (con->isSSLConnection){
   	 SSL_free(con->ssl_socket);
	 SSL_CTX_free(con->ctx);
        
    }
    close(con->socket);
    con->socket = 0;
}
