#include <stdio.h>
#include <stdlib.h>
#include"psql.h"
#include<string.h>

void pqErr(PGconn *conn, PGresult *res) {
    
    fprintf(stderr, "pqErr: %s\n", PQerrorMessage(psql_conn));    

    PQclear(res);
  //  PQfinish(psql_conn);    
    
    //exit(1);
}

int init_psql(void){
    psql_conn = PQconnectdb("user="PSQL_USER" dbname="PSQL_DB" password="PSQL_PASS);
    if (PQstatus(psql_conn) == CONNECTION_BAD) {
        
        fprintf(stderr, "Connection to database failed: %s\n",
        PQerrorMessage(psql_conn));
	return 0;
    }
    PGresult *res = 
	PQexec(psql_conn, "CREATE TABLE IF NOT EXISTS quotes (id SERIAL, channel varchar(255), author varchar(255), msg TEXT);");    
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        pqErr(psql_conn, res);
    }  
    
    return 1;
}

long long getQuotesLength(const char * channel){
    char *stm = "SELECT COUNT(*) FROM quotes where channel= $1";
    const char *paramValues[1];
    //int         paramLengths[1];
    //int         paramFormats[1];
    paramValues[0]=channel;

    PGresult * res = PQexecParams(psql_conn,
                       stm,
                       1,       /* one param */
                       NULL,    /* let the backend deduce param type */
                       paramValues,
                       NULL,    /* don't need param lengths since text */
                       NULL,    /* default to all text params */
                       0);      /* ask for binary results */




    if (PQresultStatus(res) != PGRES_TUPLES_OK){
        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(psql_conn)); 
        pqErr(psql_conn, res);
	return 0;
    }
//    printf("%s\n", PQgetvalue(res, 0, 0));// from res ROW table_column
    long long tmp= atoll(PQgetvalue(res, 0, 0));
    PQclear(res);   
    return tmp;
}

int
addQuote(const char * channel, const char * author, const char * msg){
    char *stm = "INSERT INTO quotes(channel, author, msg) VALUES($1, $2, $3);";
    const char *paramValues[3];
  //  int         paramLengths[3];
  // int         paramFormats[3];
    paramValues[0]=channel;
    paramValues[1]=author;
    paramValues[2]=msg;

    PGresult * res = PQexecParams(psql_conn,
                       stm,
                       3, //3 params
                       NULL,    /* let the backend deduce param type */
                       paramValues,
                       NULL,    /* don't need param lengths since text */
                       NULL,    /* default to all text params */
                       0);      /* ask for binary results */


    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK){
       fprintf(stderr, "SELECT failed: %s", PQerrorMessage(psql_conn)); 
        pqErr(psql_conn, res);
	return 0;
    }
    return 1;

}
//
//void
//getQuote(const char * id, const char * chn, char * rBuf){
//    char *stm = "SELECT COUNT * FROM quotes where channel=$1 AND id=$2";
//    const char *paramValues[3];
//  //  int         paramLengths[3];
//  // int         paramFormats[3];
//    paramValues[0]=chn;
//    paramValues[1]=id;
//
//
//    PGresult * res = PQexecParams(psql_conn,
//                       stm,
//                       2, //2 params
//                       NULL,    /* let the backend deduce param type */
//                       paramValues,
//                       NULL,    /* don't need param lengths since text */
//                       NULL,    /* default to all text params */
//                       0);      /* ask for binary results */
//
//
//    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK){
//        fprintf(stderr, "SELECT failed: %s", PQerrorMessage(psql_conn)); 
//        pqErr(psql_conn, res);
//	return 0;
//    }
//    return 1;
//
//}

int main() {
    



    init_psql();
    printf("%d\n",getQuotesLength("#ru_notexist"));
    addQuote("#ru_notexist", "t", "ttt");

    PQfinish(psql_conn);

    return 0;
}
