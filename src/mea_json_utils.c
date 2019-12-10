#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "mea_json_utils.h"
#include "mea_file_utils.h"
#include "mea_verbose.h"


int writeJson(char *file, cJSON *j)
{
   FILE *fp;
   int ret=-1;

   char *s = cJSON_Print(j);
   if(!s)
      return -1;
      
   fp=fopen(file, "w");
   if(fp) {
      fwrite(s, strlen(s), 1, fp);
      fclose(fp);
      ret=0;
   }
   else {
      perror(file);
   }

   free(s);

   return ret;
}


cJSON *loadJson_alloc(char *file)
{
   FILE *fp;
   char *data;

   fp = fopen(file,"r");
   if(!fp)
      return NULL;

   fseek(fp, 0L, SEEK_END);
   int32_t sz = (uint32_t)ftell(fp);
   if(sz>=0) {
      data=(char *)malloc(sz);
      if(!data)
         return NULL;
         
      fseek(fp, 0L, SEEK_SET);
      uint32_t l=(uint32_t)fread(data, sz, 1, fp);
      fclose(fp);
      
      cJSON *j=NULL;
      if(l>0) {
         j=cJSON_Parse((const char *)data);
      }
      else {
         j=NULL;
      }
      free(data);
      data=NULL;
      return j;
   }
   else {
      return NULL;
   }
}


int backupJson(char *filename)
{
   char _time[40];
   char filenamebak[1024];
   
   time_t timestamp = time(NULL); 
   
   strftime(_time, sizeof(_time)-1, "%Y-%m-%d_%X", localtime(&timestamp));
   strcpy(filenamebak, filename);
   strcat(filenamebak, ".");
   strcat(filenamebak, _time);

   return mea_filecopy(filename, filenamebak);
}