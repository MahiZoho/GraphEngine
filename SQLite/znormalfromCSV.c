#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "sqlite3.h"
#include "sqliteInt.h"
#include "vdbe.h"
#include "vdbeInt.h"
#include "opcodes.h"

struct dbdata{
    char** rowdata;
    int columncount;
};
typedef struct dbdata dbdata;

int getTableData(char *fileName, dbdata **dataArrayPtr);

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
  Vdbe *pVdbe;
  const unsigned char *zAns;

  //open database
  rc=sqlite3_open("znormal30.db",&db);
  if(rc) {
    fprintf(stderr,"cant open database: %s \n",sqlite3_errmsg(db));
    exit(0);
  }
  
  //Create Table...
  sql="CREATE TABLE Referrals(c1 integer primary key, c2 text, c3 text);";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, " here SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }

  printf("####################### Inserting rows #############################\n");
  
    char *fileName = "/Volumes/Official/ZLabs/Pali/referraldata.csv";
    dbdata *dataArray = (dbdata*)calloc(1000,sizeof(dbdata));
    int rowcount = getTableData(fileName, &dataArray);
    //printing data...
    for(int a = 0; a <  rowcount; a++){
        char **tmprowdata = dataArray[a].rowdata;
//        int columncount = dataArray[a].columncount;
//        for(int b = 0; b < columncount; b++){
//            printf("%s\t", tmprowdata[b]);
//        }
//        printf("\n");
        
        char *c1Data = tmprowdata[0];
        char *c2Data = tmprowdata[1];
        char *c3Data = tmprowdata[2];
        char *sql;
        char *prefix = "INSERT INTO Referrals VALUES(";
        char *delimeter = ",'";
        char *quot = "'";
        char *suffix = "');";
        
        int sqllen = (int)(strlen(prefix)+strlen(c1Data)+strlen(delimeter)+strlen(c2Data)+strlen(quot)+strlen(delimeter)+strlen(c3Data)+strlen(suffix)+1);
        sql = (char*)malloc(sizeof(char)*(sqllen));
        snprintf(sql, sqllen, "%s%d%s%s%s%s%s%s", prefix,atoi(c1Data),delimeter,c2Data,quot,delimeter,c3Data,"');'");
        printf("%s\n",sql);
        rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
        if( rc != SQLITE_OK ){
          fprintf(stderr, " here SQL error: %s\n", zerrmsg);
          sqlite3_free(zerrmsg);
        }
    }

  printf("####################### Inserted 3 rows #############################\n");
  

  printRocksDBData(db);


  printf("####################### Read from table where c1 =  #############################\n");

  sql="select * from Referrals where c1 = 1;";
  rc=sqlite3_exec(db,sql,callback,0,&zerrmsg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, " here SQL error: %s\n", zerrmsg);
    sqlite3_free(zerrmsg);
  }
  printf("####################### Read finished #############################\n");
  

  sqlite3_close(db);
}


int getTableData(char *fileName, dbdata **dataArrayPtr){
    dbdata *dataArray = dataArrayPtr[0];
    int rowcount = 0;
    FILE* stream = fopen(fileName, "r");
    char line[1024];
    
    while (fgets(line, 1024, stream))
    {
        char* tmp = strdup(line);
        int columncount = 0;
        char** tmprowdata = (char **)calloc(10,sizeof(char *));
        printf("1024 byte data read is %s\n", tmp);
        
        
        
        char *tok = strtok(line, ",");
        //        printf("111 %s\n", tok);
        
        // char* tok = strtok(line, ",");
        while(tok != NULL){
            // printf("%s\t", tok);
            char *coldata = malloc(sizeof(char)*100);
            memcpy(coldata, tok, strlen(tok)+1);
            tmprowdata[columncount] = coldata;
            
            tok = strtok(NULL, ",\n");
            //            printf("222 %s\n", tok);
            //            memcpy(coldata, tok, strlen(tok)+1);
            columncount++;
        }
        // printf("\n");
        dataArray[rowcount].rowdata = tmprowdata;
        dataArray[rowcount].columncount = columncount;
        rowcount++;
        free(tmp);
    }
    
    //printing data...
//    for(int a = 0; a <  rowcount; a++){
//        char **tmprowdata = dataArray[a].rowdata;
//        int columncount = dataArray[a].columncount;
//        for(int b = 0; b < columncount; b++){
//            printf("%s\t", tmprowdata[b]);
//        }
//        printf("\n");
//    }
    return rowcount;
}