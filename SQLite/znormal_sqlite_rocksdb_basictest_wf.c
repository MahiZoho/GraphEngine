#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "sqlite3.h"
#include "sqliteInt.h"
#include "vdbe.h"
#include "vdbeInt.h"
#include "opcodes.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
  int i;
  char *zColumnSelector = (char*)NotUsed;
  if(zColumnSelector != NULL)
    printf("%s,",zColumnSelector);
    for (i = 0;i<argc; i++) {  printf("%s,",argv[i] ? argv[i] : "NULL");     
}
  printf("\n");
  return 0;
}

int Custom_commitCallback(Vdbe*);

int main(int argc,char* argv[]) {

  sqlite3 *db;
  sqlite3_stmt *pStmt = NULL;
  char *sql,*zerrmsg=0;
  int rc;
  //long int i,j,w;
  Vdbe *pVdbe;
  const unsigned char *zAns;

  //open database
  rc=sqlite3_open("znormal30.db",&db);
  if(rc) {
    fprintf(stderr,"cant open database: %s \n",sqlite3_errmsg(db));
    exit(0);
  }

  /*sql="CREATE TABLE mahi(c1 integer primary key, c2 integer);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, " here SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }

  printf("####################### Inserting 3 rows #############################\n");
  sql="insert into mahi values(100,436);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ) {
    fprintf(stderr, "SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }

  sql="insert into mahi values(200,4345);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ) {
    fprintf(stderr, "SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }

  sql="insert into mahi values(300,567);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ) {
    fprintf(stderr, "SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }
  printf("####################### Inserted 3 rows #############################\n");*/

  sql="CREATE TABLE mahi(c1 integer primary key, c2 integer, c3 text, c4 text, c5 text);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, " here SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }

  printf("####################### Inserting 3 rows #############################\n");

  long int i,j,w,m;
  int l1,l2,l3,l4,l5,lfin;
  char *c1,*c2,*c3;
  c1 = "INSERT INTO mahi VALUES(";
  c2 = ",";
  c3 = ",'1abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz', '2abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz', '3abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz');";
  l1 = strlen(c1);

  l3 = strlen(c2);

  l5 = strlen(c3);

  for(i=1,j=2;i<=20;i++,j++) {
    l2 = snprintf(0,0,"%ld",i);
    l4 = snprintf(0,0,"%ld",j);
    lfin = l1+l2+l3+l4+l5;
    printf("j=%ld\n",j);
    sql = (char*)malloc(sizeof(char)*(lfin+1));
    snprintf(sql,lfin + 1,"%s%ld%s%ld%s",c1,i,c2,j,c3);
    //printf("%s\n",sql);
    rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
    if( rc != SQLITE_OK ){
      fprintf(stderr, " here SQL error: %s\n", zerrmsg);
      sqlite3_free(zerrmsg);
    }
  }

  printf("####################### Inserted 3 rows #############################\n");
  
  // Custom_PopulatePageInfo(db);
  printRocksDBData(db);


  printf("####################### Read from mahi where c1 =  #############################\n");

  sql="select * from mahi where c1 = 1;";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, " here SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }
  printf("####################### Read finished #############################\n");

  sqlite3_close(db);
}