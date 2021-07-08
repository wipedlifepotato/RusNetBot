#include"irc.h"
static pingServ(struct IRCConnection * con){
	while(con->socket){
		sleep(30);
		irc_sendMsg(*con, "PING");
	}
}

int main(int count, char *strings[])
{
    if ( count != 7 )
    {
        printf("usage: %s <hostname> <portnum> <nick> <username> <realname> <ssl >1/0>\n", strings[0]);
        exit(0);
    }
    if(!init_psql())abort();
    bool ssl = atoi(strings[6]) > 0 ? true : false;
    while(1){
    	struct IRCConnection con = OpenConnection(strings[1], atoi(strings[2]), ssl);
    	regOnServ(con, strings[3], strings[4], strings[5]);
    	joinChn(con, "#mogi","#magi","#ru");
	pthread_t p;
	pthread_create(&p, NULL, pingServ, (void*)&con);
    	recvHandler(con);
    	freeConnect(&con);
	pthread_exit(&p);
    }
    return 0;
}
