#include"irc.h"
#include<pthread.h>
#include<iniparser/iniparser.h>
#define TMP_BUF 1024
struct thread_data{
	const char host[TMP_BUF];
	int port;
	int ssl;
	const char username[TMP_BUF];
	const char nickname[TMP_BUF];
	const char realname[TMP_BUF];
};
void* doConnect(void * data){
	struct thread_data * d = data;
	struct thread_data copy = *d;
	//        printf("usage: %s <hostname> <portnum> <nick> <username> <realname> <ssl >1/0>\n", strings[0]);
	struct IRCConnection con = OpenConnection(copy.host, copy.port, copy.ssl);
    	regOnServ(con, copy.nickname, copy.username, copy.realname);
    	joinChn(con, "#ru","#mogi","#magi");
    	recvHandler(con);

    	freeConnect(&con);
}

int main(int count, char *strings[])
{
    if(!init_psql())abort();
    dictionary *dict= iniparser_load("servers.ini");
    int count_servers = iniparser_getnsec (dict);
    const int cs = count_servers;
    printf("count servers: %d\n", count_servers);
    pthread_t thread[count_servers];
    struct thread_data data[count_servers];
    while(count_servers){
		const char * serv=
			iniparser_getsecname (dict, count_servers-1);
		char buf[1024];
		sprintf(buf, "%s:host", serv);
		const char * host =
			iniparser_getstring (dict, buf, "127.0.0.1");
		printf("Serv: %s; host: %s\n", serv, host);
		sprintf(buf, "%s:port", serv);
		const char * port =
			iniparser_getstring (dict, buf, "6667");
		printf("Serv: %s; port: %s\n", serv, port);


		sprintf(buf, "%s:ssl", serv);
		const char * ssl =
			iniparser_getstring (dict, buf, "0");
		printf("Serv: %s; ssl: %s\n", serv, ssl);
		
		sprintf(buf, "%s:username", serv);
		const char * username =
			iniparser_getstring (dict, buf, "koshkobot");
		printf("Serv: %s; username: %s\n", serv, ssl);

		sprintf(buf, "%s:nick", serv);
		const char * nick =
			iniparser_getstring (dict, buf, username);
		printf("Serv: %s; nick: %s\n", serv, nick);

		sprintf(buf, "%s:realname", serv);
		const char * realname =
			iniparser_getstring (dict, buf, username);
		printf("Serv: %s; realname: %s\n", serv, realname);
/*
struct thread_data{
	char * host;
	int port;
	int ssl;
	char * username;
	char * nickname;
	char * realname;
};
*/
		/*strcpy(data[count_servers-1].host, host);//={host,port,atoi(ssl), username,nickname,realname};
		data[count_servers-1].port=atoi(port);
		data[count_servers-1].ssl=atoi(ssl);
		strcpy(data[count_servers-1].username,username);
		strcpy(data[count_servers-1].nickname,nick);
		strcpy(data[count_servers-1].realname,realname);
		pthread_create(&thread[count_servers-1],NULL, doConnect, &data);*/
		
		count_servers--;
		
     }
  //   for(int i =0; i < cs; i++){
//	puts("join");
//	pthread_join(thread[i], NULL);
    // }
if ( count != 7 )
    {
        printf("usage: %s <hostname> <portnum> <nick> <username> <realname> <ssl >1/0>\n", strings[0]);
        exit(0);
    }
    if(!init_psql())abort();
    bool ssl = atoi(strings[6]) > 0 ? true : false;
    struct IRCConnection con = OpenConnection(strings[1], atoi(strings[2]), ssl);


    regOnServ(con, strings[3], strings[4], strings[5]);
    joinChn(con, "#ru","#mogi","#magi");
    recvHandler(con);
    freeConnect(&con);
    return 0;
}
