#include <postgresql/libpq-fe.h>
#define PSQL_USER "rusnet"
#define PSQL_PASS "123"
#define PSQL_DB "rusnet"
static PGconn * psql_conn;
void pqErr(PGconn *conn, PGresult *res);
int init_psql(void);
