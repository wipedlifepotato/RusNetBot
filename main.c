#include"irc.h"

int main(int count, char *strings[])
{
    if ( count != 6 )
    {
        printf("usage: %s <hostname> <portnum> <nick> <username> <realname>\n", strings[0]);
        exit(0);
    }
    struct IRCConnection con = OpenConnection(strings[1], atoi(strings[2]), true);

    regOnServ(con, strings[3], strings[4], strings[5]);
    joinChn(con, "#mogi","#magi");
    recvHandler(con);
    freeConnect(&con);
    return 0;
}
