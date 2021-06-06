#include"irc.h"

int main(int count, char *strings[])
{
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
