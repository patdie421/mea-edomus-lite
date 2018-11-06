#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "curl_adds.h"

#include "mea_verbose.h"


int curl_result_init(struct curl_result_s *cr)
{
   cr->l = 0;
   cr->p = NULL;

   return 0;
}


int curl_result_release(struct curl_result_s *cr)
{
   cr->l=0;
   if(cr->p)
   {
      free(cr->p);
      cr->p=NULL;
   }
   
   return 0;
}


size_t curl_result_get(void *ptr, size_t size, size_t nmemb, struct curl_result_s *cr)
{
   char *tmp;
   size_t new_len = cr->l + size*nmemb;

   tmp = realloc(cr->p, new_len+1);

   if (tmp == NULL)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : realloc() failed\n", DEBUG_STR, __func__);
      return -1;
   }
   else
      cr->p=tmp;

  memcpy(cr->p+cr->l, ptr, size*nmemb);
  cr->p[new_len] = '\0';
  cr->l = new_len;

  return size*nmemb;
}
