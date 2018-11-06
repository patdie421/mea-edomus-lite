#ifndef __curl_adds_h
#define __curl_adds_h

#include <curl/curl.h>

#include "curl_adds.h"

struct curl_result_s
{
   char *p;
   size_t l;
};

int curl_result_init(struct curl_result_s *cr);
int curl_result_release(struct curl_result_s *cr);
size_t curl_result_get(void *ptr, size_t size, size_t nmemb, struct curl_result_s *cr);

#endif