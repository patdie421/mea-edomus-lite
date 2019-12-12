#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "users.h"

#include "configuration.h"
#include "mea_verbose.h"
#include "mea_json_utils.h"

char *_users2 = "{ \
\"admin\": { \"password\":\"changeme\", \"fullname\":\"administrator\", \"profile\":1 } \
}";
cJSON *users2 = NULL;
char *usersfullpath=NULL;


char *user_getFileFullPath()
{
   return usersfullpath;
}


int initUsers()
{
   char *meapath=appParameters_get("MEAPATH", NULL);
   char *usersfile=appParameters_get("USERSFILE", NULL);
   char *etc="/etc/";
   int usersfullpath_l=(int)(strlen(meapath)+strlen(etc)+strlen(usersfile)+1);

   if(usersfullpath) {
      free(usersfullpath);
   }
   usersfullpath=malloc(usersfullpath_l);
   
   strcpy(usersfullpath, meapath);
   strcat(usersfullpath, etc);
   strcat(usersfullpath, usersfile);
   
   users2=loadJson_alloc(usersfullpath);
   if(!users2) {
      users2=cJSON_Parse(_users2);
      // if(users2) {
      //   writeJson(usersfullpath, users2);
      // }
   }
   
   return 0;
}
