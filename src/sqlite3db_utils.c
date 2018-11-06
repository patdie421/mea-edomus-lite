//
//  sqlite3db_utils.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 22/10/13.
//
//
#include "sqlite3db_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#include <sqlite3.h>

#include "mea_verbose.h"


int16_t sqlite3_dropTable(sqlite3 *sqlite3_param_db, char *table)
{
    char sql_query[1024];
    char *errmsg = 0;
    
    int16_t n=snprintf(sql_query,sizeof(sql_query),"DROP TABLE IF EXISTS '%s'",table);
   if(n<0 || n==sizeof(sql_query))
      return 0;

    int16_t func_ret = sqlite3_exec(sqlite3_param_db, sql_query, NULL, NULL, &errmsg);
    if( func_ret != SQLITE_OK )
    {
        DEBUG_SECTION mea_log_printf("%s (%s) : sqlite3_exec - %s\n", DEBUG_STR, __func__, errmsg);
        sqlite3_free(errmsg);
        return 1;
    }
    else 
        return 0;
}


int16_t sqlite3_tableExist(sqlite3 *sqlite3_param_db, char *table)
{
    sqlite3_stmt * stmt;
    char sql_query[1024];
    
    int16_t n=snprintf(sql_query,sizeof(sql_query),"SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'", table);
    if(n<0 || n==sizeof(sql_query))
        return -1;

    int16_t func_ret = sqlite3_prepare_v2(sqlite3_param_db, sql_query, n+1, &stmt, NULL);
    if(!func_ret)
    {
        int16_t s = sqlite3_step (stmt);
        if (s == SQLITE_ROW)
        {
            int16_t nb_tables=sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return nb_tables;
        }
        else if (s==SQLITE_ERROR)
        {
            DEBUG_SECTION mea_log_printf("%s (%s) : sqlite3_step - %s\n", DEBUG_STR,__func__, sqlite3_errmsg(sqlite3_param_db));
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    else
    {
        DEBUG_SECTION mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", DEBUG_STR, __func__, sqlite3_errmsg(sqlite3_param_db));
    }
    return -1;
}


int16_t sqlite3_dropDatabase(char *db_path)
{
    return unlink(db_path);
}


int16_t sqlite3_doSqlQueries(sqlite3 *sqlite3_db, char *queries[])
{
   int16_t rc=0;

   for(int16_t i=0;queries[i];i++)
   {
      char *errmsg = NULL;
      
      int16_t ret = sqlite3_exec(sqlite3_db, queries[i], NULL, NULL, &errmsg);
      if( ret != SQLITE_OK )
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : sqlite3_exec - %s\n", DEBUG_STR, __func__, errmsg);
         sqlite3_free(errmsg);
         rc=1;
         break;
      }
   }
   return rc;
}
