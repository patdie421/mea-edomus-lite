#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "mea_json_utils.h"

int writeJson(char *file, cJSON *j)
{
   FILE *fp;
   int ret=-1;

   char *s = cJSON_Print(j);

   printf("write %s\n", file);
   printf("%s\n", s);

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


cJSON *loadJson(char *file)
{
   FILE *fp;
   char *data;

   fp = fopen(file,"r");
   if(!fp)
      return NULL;

   fseek(fp, 0L, SEEK_END);
   uint32_t sz = (uint32_t)ftell(fp);
   if(sz>=0) {
      data=(char *)malloc(sz);
      fseek(fp, 0L, SEEK_SET);
      uint32_t l=fread(data, sz, 1, fp);
      fclose(fp);
      if(l>0) {
         return cJSON_Parse((const char *)data);
      }
      else {
         return NULL;
      }
   }
   else {
      return NULL;
   }
}

