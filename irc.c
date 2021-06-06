#include"irc.h"
#include"curl.h"
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
    if(useSSL)
	if(!ssl_inited){
		SSL_library_init();
		ssl_inited=true;
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
    }//
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
	if(c.isSSLConnection)
		return SSL_read(c.ssl_socket, buf, n); 
	return read(c.socket, buf, n);
}

void 
pingIfNeed(ircc c, const char * last_buf){
		char * pPing;
		char buf[BUF_SIZE];
		if( (pPing=strstr(last_buf, "PING :")) != 0 ){
			//puts("PING - PONG");
			//PING :ZASDFDFSDF\n
			pPing += 6;
			//printf("%s\n",pPing);
			char * tmp = pPing;
			while(*(pPing) != '\n' && *(pPing) != '\0') *(pPing++);
			if(*pPing !='\n') return;
			char pong[(pPing-tmp)+1];
			memcpy(pong, tmp, (pPing-tmp));
			pong[(pPing-tmp)+1]='\0';

			sprintf(buf, "PONG %s\r\n", pong);
			irc_sendMsg(c, buf);
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
		free((void*) (what[n-1]) );
	}
	free((void*)what);
}

void 
PRIVMSG(ircc c, const char * chnusr, const char * msg, const unsigned char color){
	char buf[BUF_SIZE];
	bzero(buf, BUF_SIZE);
	char color_buf[3];
	if(color != 0){
		color_buf[0]='';
		color_buf[1]=color;
		color_buf[2]='\0';
	}else{
		color_buf[0]='\0';
		color_buf[1]='\0';
	}
	sprintf(buf,"PRIVMSG %s :%s%s\r\n",chnusr, color_buf, msg);
	printf("buf: %s\n", buf);
	irc_sendMsg(c,buf);
}
void 
msg_handler(ircc c, const char ** splitted, size_t splitted_size){
	/*if(!regex_inited){
		if(regcomp(&regex_url, URL_REGEX, 0) != 0) abort();
	}*/
	const char *sender, *channel;
	sender=splitted[0];
	channel=splitted[2];
	splitted[3] = splitted[3]+1;
	char nickSender[BUF_SIZE];
	memcpy(nickSender, (sender+1), (strstr(sender,"!") -(sender+1)));

	//printf("%s send msg in %s -> ", sender, channel);
	#define UNALLOWED {i++;continue;}
	size_t i = 3;
	char buf[BUF_SIZE];
	while( i < splitted_size ){
		//printf("%s %d\n", splitted[i], i);
		//if( regexec(&regex_url, splitted[i], 0, NULL, 0) == 0 ){
		if(strstr(splitted[i], "http") != NULL){
			if( strstr(splitted[i], "127.0.0.1") != NULL) UNALLOWED;
			if( strstr(splitted[i], "192.168.") != NULL) UNALLOWED;
			//printf("Url: %s\n", splitted[i]);
			char about_page[BUF_SIZE];
			char buf[BUF_SIZE];
			get_info_about_url(splitted[i], about_page);
			printf("About_Page: %s\n", about_page);
			PRIVMSG(c, channel, about_page, 2);
		}else if( (strstr(splitted[i], "!aq") != NULL) && i == 3){
			i++;
			while(i < splitted_size){
				sprintf(buf, "%s ", splitted[i++]);
			}
			if(addQuote(channel, nickSender, buf)){
				sprintf(buf, "%s цитата добавлена,у неё должен быть номер %d\n", nickSender, getQuotesLength());
				PRIVMSG(c, channel, buf, 0);
			}

		}else if( (strstr(splitted[i], "!q") != NULL) && i == 3){
			char buf[BUF_SIZE];
			i++;
			if( atoll(splitted[i]) <= 0)
				sprintf(buf, 
				"%s, номер цитаты дан не правильным должен быть от 1-%s, а он->%lu"
					, nickSender, getQuotesLength(), atoll(splitted[i]));
			else
				getQuote(splitted[i], buf);
			
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
    char buf[BUF_SIZE];
    size_t rcvBytes;
    do{
	bzero(buf, BUF_SIZE);
	rcvBytes=irc_recvMsg(c, buf, BUF_SIZE);
	printf("Recv: %s\n", buf);
	pingIfNeed(c,buf);

	size_t size;
	char ** splitted = split_msg(buf, ' ', &size);
	//puts("Splitted:");
	for(unsigned int i =0; i<size;i++){
		//strcasecmp ignore case
		if(strcmp(splitted[i], "PRIVMSG") == 0){
			msg_handler(c, (const char**)splitted, size);
		}
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
}
