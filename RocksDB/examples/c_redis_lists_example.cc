#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "rocksdb/c.h"

#include <unistd.h>  // sysconf() - get CPU count

const char DBPath[] = "/tmp/rocksdb_c_redis_example";
const char DBBackupPath[] = "/tmp/rocksdb_c_redis_example_backup";

typedef enum { false, true } bool;

// rocksdb_column_family_handle_t* getColumnFamilyHandle(rocksdb_column_family_handle_t **handles, char **column_fams, char *column_family_name, const size_t cflen);
// bool getColumnFamilyHandle(rocksdb_column_family_handle_list_t *&columnFamilyHandleList, rocksdb_column_family_handle_t *&columnFamilyHandle, char *column_family_name);

int main(int argc, char **argv) {
  rocksdb_t *db;
  rocksdb_backup_engine_t *be;
  rocksdb_redis_lists_t *redisLists;
  rocksdb_column_family_handle_struct_array_t* columnFamilyHandleList;

  //Create Options...
  rocksdb_options_t *options = rocksdb_options_create();

  // Optimize RocksDB. This is the easiest way to
  // get RocksDB to perform well
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);  // get # of online cores
  rocksdb_options_increase_parallelism(options, (int)(cpus));
  rocksdb_options_optimize_level_style_compaction(options, 0);
  // create the DB if it's not already present
  rocksdb_options_set_create_if_missing(options, 1);


  // open DB
  char *err = NULL;
  db = rocksdb_open(options, DBPath, &err);
  assert(!err);
  rocksdb_close(db);

  // Loading Column Families..
  // char *err = NULL;
  err = NULL;
  size_t cflen;
  char** column_fams = rocksdb_list_column_families(options, DBPath, &cflen, &err);
  if(err){
    printf("Error while listing column families = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;

  for(int a = 0; a<cflen; a++){
    printf("columnfamily[%d] = %s\n", a, column_fams[a]);
  }
  printf("-------------\n");
  

  //Open DB..
  // columnFamilyHandle = new rocksdb_column_family_handle_t;
  // columnFamilyHandleList = new rocksdb_column_family_handle_list_t;
  db = rocksdb_open_column_families_default_options_zlabs(options, DBPath, cflen, column_fams, &columnFamilyHandleList, &err);
  if(err){
    printf("Error while opening the RocksDB = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;

  //De-Allocating ColumnFamily Array...
  rocksdb_list_column_families_destroy(column_fams, cflen);
  

  // open Backup Engine that we will use for backing up our database
  be = rocksdb_backup_engine_open(options, DBBackupPath, &err);
  if(err){
    printf("Error while opening Backup Engine = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;

  //Create Read and Write Options...
  rocksdb_writeoptions_t *writeoptions = rocksdb_writeoptions_create();
  rocksdb_readoptions_t *readoptions = rocksdb_readoptions_create();

  //Create Key-Value(List) Object...
  redisLists = rocksdb_redis_lists_create(DBPath, db, writeoptions, readoptions);


  //Get or Create ColumnFamilyHandle..
  char *column_family_name = "new_col_family_9";
  // char *column_family_name = "new_column_family";
  rocksdb_column_family_handle_t *columnFamilyHandle;
  columnFamilyHandle = rocksdb_GetColumnFamilyHandle(columnFamilyHandleList, column_family_name);
  if(!columnFamilyHandle){
    printf("creating new column family....\n");
    columnFamilyHandle = rocksdb_create_column_family_zlabs(db, options, column_family_name, &columnFamilyHandleList, &err);

    if(err){
      printf("Error while creating column family = %s\n", err);
    }
    assert(!err);
    free(err);
    err = NULL;



    column_fams = rocksdb_list_column_families(options, DBPath, &cflen, &err);
    if(err){
      printf("Error while listing column families = %s\n", err);
    }
    assert(!err);
    free(err);
    err = NULL;

    for(int a = 0; a<cflen; a++){
      printf("columnfamily[%d] = %s\n", a, column_fams[a]);
    }
    printf("-------------\n");
    rocksdb_list_column_families_destroy(column_fams, cflen);
  }

  //Put data into Key-Value(List)..
  char *key = "100";
  char *value1 = "1";
  char *value2 = "2";
  char *value3 = "3";
  char *value4 = "4";
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value1, &err);
  if(err){
    printf("Error while Pushing Data to List = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value2, &err);
  assert(!err);
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value3, &err);
  assert(!err);

  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value4, &err);
  assert(!err);
  
  //Reading data from Key-Value(List)..
  printf("------ Printing Key1 data after insertion --------\n");
  rocksdb_redis_lists_print(redisLists, columnFamilyHandle, key);



  key = "200";
  value1 = "5";
  value2 = "6";
  value3 = "4";
  value4 = "3";
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value1, &err);
  assert(!err);
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value2, &err);
  assert(!err);
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value3, &err);
  assert(!err);

  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value4, &err);
  assert(!err);
  
  //Reading data from Key-Value(List)..
  printf("------ Printing Key2 data after insertion --------\n");
  rocksdb_redis_lists_print(redisLists, columnFamilyHandle, key);



  printf("####### Inserting Second Time ########\n");
  //Get or Create ColumnFamilyHandle..
  column_family_name = "new_col_family_9";
  columnFamilyHandle = rocksdb_GetColumnFamilyHandle(columnFamilyHandleList, column_family_name);
  if(!columnFamilyHandle){
    printf("creating new column family....\n");
    columnFamilyHandle = rocksdb_create_column_family_zlabs(db, options, column_family_name, &columnFamilyHandleList, &err);

    if(err){
      printf("the err = %s\n", err);
    }
    assert(!err);
    free(err);
    err = NULL;
  }

  //Put data into Key-Value(List)..
  key = "300";
  value1 = "10";
  value2 = "20";
  value3 = "30";
  value4 = "40";
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value1, &err);
  assert(!err);
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value2, &err);
  assert(!err);
  
  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value3, &err);
  assert(!err);

  rocksdb_redis_lists_push_left(redisLists, columnFamilyHandle, key, value4, &err);
  assert(!err);
  
  //Reading data from Key-Value(List)..
  printf("------ Printing Key1 data after insertion --------\n");
  // char **listdata = malloc(2 * sizeof(char*));
  // int datacount = rocksdb_redis_lists_get_data(redisLists, columnFamilyHandle, key, &listdata, &err);
  // printf("datacount = %d\n", datacount);
  // rocksdb_redis_lists_print(redisLists, columnFamilyHandle, key);
  rocksdb_data_t *listdata = rocksdb_redis_lists_get_data(redisLists, columnFamilyHandle, key, &err);
  if(err){
      printf("Error while getting ListData = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;

  printf("Local Print...\n");
  char **dataArray = listdata->data;
  for(int k=0; k<listdata->size; k++){
    printf("%s\t", dataArray[k]);
  }
  printf("\n");

  rocksdb_destroy_data(listdata);




  printf("------------------------------------------------\n");
  rocksdb_data_t *commondata = rocksdb_redis_lists_get_common_data(redisLists, columnFamilyHandle, "100", "200", &err);
  if(err){
      printf("Error while getting CommonData = %s\n", err);
  }
  assert(!err);
  free(err);
  err = NULL;

  char **commondataArray = commondata->data;
  for(int x=0; x<commondata->size; x++){
    printf("%s\t", commondataArray[x]);
  }
  printf("\n");

  rocksdb_destroy_data(commondata);
  printf("------------------------------------------------\n");



  // create new backup in a directory specified by DBBackupPath
  rocksdb_backup_engine_create_new_backup(be, db, &err);
  if(err){
    printf("the err = %s\n", err);
  }
  assert(!err);

  //Closing DB..
  // rocksdb_close(db);

  // If something is wrong, you might want to restore data from last backup
  // rocksdb_restore_options_t *restore_options = rocksdb_restore_options_create();
  // rocksdb_backup_engine_restore_db_from_latest_backup(be, DBPath, DBPath,
  //                                                     restore_options, &err);
  // if(err){
  //   printf("the err = %s\n", err);
  // }
  // assert(!err);
  // rocksdb_restore_options_destroy(restore_options);

  

  // ############## cleanup ####################
  //DeAllocating ColumnFamilyHandleList...
  // int col_fm_count = columnFamilyHandleList->cflen;
  // rocksdb_column_family_handle_t **cf_handle_struct_array = columnFamilyHandleList->rep;
  // for(int a = 0; a < col_fm_count; a++){
  //   rocksdb_column_family_handle_destroy(cf_handle_struct_array[a]);
  // }
  // free(cf_handle_struct_array);

  rocksdb_column_family_handle_list_destroy(columnFamilyHandleList);
  rocksdb_writeoptions_destroy(writeoptions);
  rocksdb_readoptions_destroy(readoptions);
  rocksdb_options_destroy(options);
  rocksdb_backup_engine_close(be);
  rocksdb_close(db);
  rocksdb_redis_lists_close(redisLists);

  return 0;
}

// bool getColumnFamilyHandle(rocksdb_column_family_handle_list_t *&columnFamilyHandleList, rocksdb_column_family_handle_t **columnFamilyHandle, char *column_family_name){
//     int i;
//     bool found = false;
//     ColumnFamilyHandle *handle;

//     ColumnFamilyHandle **&cf_handleList = columnFamilyHandleList->rep;
//     int cflen = cf_handle_list_t->cflen;

//     for(i=0; i<cflen; i++){
//       handle = cf_handleList[i];
//       const char *name = (handle->GetName()).c_str();
//       if(strcmp(column_family_name, name) == 0){
//         found = true;
//         break;
//       }
//     }
//     if(found == true){
//       *columnFamilyHandle->rep = handle;
//     }
//     return found;
// }
