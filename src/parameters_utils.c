//
//  parameters_mgr.c
//
//  Created by Patrice DIETSCH on 24/10/12.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include "uthash.h"

#include "macros.h"
#include "mea_verbose.h"

#include "parameters_utils.h"
#include "mea_string_utils.h"


static void _clean_parsed_parameters(parsed_parameters_t *params);


#ifdef PARSED_PARAMETERS_CACHING_ENABLED
struct _parsed_parameters_cache_elems_s
{
   char *parameters_string;
   parsed_parameters_t *parsed_parameters;

   time_t last_access;
   UT_hash_handle hh;
};


struct _parsed_parameters_cache_list_s
{
   char **parameters_to_find;
   struct _parsed_parameters_cache_elems_s *parsed_parameters_cache_elems;

   UT_hash_handle hh;
};


static struct _parsed_parameters_cache_list_s *_parsed_parameters_cache_list=NULL;
static uint32_t _parsed_parameters_cache_counter=0;

static pthread_rwlock_t *_parsed_parameters_cache_rwlock = NULL;


static int16_t _parsed_parameters_clean_cache(time_t t, int force)
{
   time_t now=time(NULL);

   if(_parsed_parameters_cache_rwlock==NULL)
      return 0;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)_parsed_parameters_cache_rwlock );
   pthread_rwlock_wrlock(_parsed_parameters_cache_rwlock);

   struct _parsed_parameters_cache_list_s *s  = NULL;
   struct _parsed_parameters_cache_list_s *s_tmp  = NULL;
 
   HASH_ITER(hh, _parsed_parameters_cache_list, s, s_tmp) {
       struct _parsed_parameters_cache_elems_s *e;
       struct _parsed_parameters_cache_elems_s *e_tmp;

       HASH_ITER(hh, s->parsed_parameters_cache_elems, e, e_tmp) {
          if(t==0 || difftime(now,e->last_access)>(double)t) {
             if(force || (e->parsed_parameters->in_use == 0) ) {
                HASH_DEL(s->parsed_parameters_cache_elems, e);
                _clean_parsed_parameters(e->parsed_parameters);
                free(e->parsed_parameters); 
                free(e);
                _parsed_parameters_cache_counter--;
             }
          }
       }

       if(HASH_COUNT(s->parsed_parameters_cache_elems) == 0) {
          HASH_DEL(_parsed_parameters_cache_list, s); 
          free(s);
       }
   }
   
_parsed_parameters_clean_cache_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);
   
   return 0;
}


static int16_t _parsed_parameters_add_to_cache(char **parameters_to_find, char *parameters_string, parsed_parameters_t *parsed_parameters)
{
   struct _parsed_parameters_cache_list_s *s  = NULL;
   struct _parsed_parameters_cache_elems_s *e = NULL;

   if(_parsed_parameters_cache_counter >= PARSED_PARAMETERS_CACHING_MAX)
      return -1;

   if(_parsed_parameters_cache_rwlock==NULL) {
#ifdef PARSED_PARAMETERS_CACHING_AUTOINIT   
      _parsed_parameters_cache_rwlock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
      if(!_parsed_parameters_cache_rwlock) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return -1;
      }
      pthread_rwlock_init(_parsed_parameters_cache_rwlock, NULL);
#else
      return -1; // pas de verrou on ne peut pas utiliser le cache
#endif
   }

   pthread_cleanup_push((void *)pthread_rwlock_unlock, _parsed_parameters_cache_rwlock);
   pthread_rwlock_wrlock(_parsed_parameters_cache_rwlock);

   HASH_FIND(hh, _parsed_parameters_cache_list, parameters_to_find, parsed_parameters->nb * sizeof(char *), s);
   if(!s) {
      s = malloc(sizeof(struct _parsed_parameters_cache_list_s));
      if(!s) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      s->parameters_to_find = parameters_to_find;
      s->parsed_parameters_cache_elems=NULL;
      
      HASH_ADD_KEYPTR(hh, _parsed_parameters_cache_list, s->parameters_to_find, parsed_parameters->nb * sizeof(char *), s );
   }

   HASH_FIND(hh, s->parsed_parameters_cache_elems, parameters_string, strlen(parameters_string), e);
   if(e) {
      DEBUG_SECTION mea_log_printf("%s (%s) : id key duplicated (%s)\n",DEBUG_STR, __func__, parameters_string);
   }
   else {
      e = malloc(sizeof(struct _parsed_parameters_cache_elems_s));
      if(!e) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      e->parameters_string = mea_string_alloc_and_copy(parameters_string);
      if(!e->parameters_string) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         free(e);
         e=NULL;
         goto _parsed_parameters_add_to_cache_clean_exit;
      }
      e->parsed_parameters = parsed_parameters;
      e->last_access=time(NULL);
      HASH_ADD_KEYPTR(hh, s->parsed_parameters_cache_elems, e->parameters_string, strlen(e->parameters_string), e );
      parsed_parameters->from_cache=1;
      _parsed_parameters_cache_counter++;
   }

_parsed_parameters_add_to_cache_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);
   
   return 0;
}


static parsed_parameters_t *_parsed_parameters_get_from_cache(char *parameters_to_find[], int nb_params, char *parameters_string)
{
   struct _parsed_parameters_cache_list_s *s = NULL;
   struct _parsed_parameters_cache_elems_s *e = NULL;
   parsed_parameters_t *ret=NULL;

   if(_parsed_parameters_cache_rwlock==NULL)
      return NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)_parsed_parameters_cache_rwlock );
   pthread_rwlock_rdlock(_parsed_parameters_cache_rwlock);
      
   HASH_FIND(hh, _parsed_parameters_cache_list, parameters_to_find, nb_params * sizeof(char *), s);
   if(!s)
      goto _parsed_parameters_get_clean_exit;
  
   HASH_FIND(hh, s->parsed_parameters_cache_elems, parameters_string, strlen(parameters_string), e);
   if(!e)
     goto _parsed_parameters_get_clean_exit;
   
   e->last_access=time(NULL);
   ret=e->parsed_parameters;
   
_parsed_parameters_get_clean_exit:
   pthread_rwlock_unlock(_parsed_parameters_cache_rwlock);
   pthread_cleanup_pop(0);  
   
   return ret;
}
#endif


int16_t is_in_assocs_list(struct assoc_s *assocs_list, int val1, int val2)
{
   for(int i=0;assocs_list[i].val1!=-1;i++)
      if(assocs_list[i].val1==val1 && assocs_list[i].val2==val2)
         return 1;
   return 0;
}


int16_t parsed_parameters_clean_all(int force)
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   return _parsed_parameters_clean_cache(0, force);
#endif
}


int16_t parsed_parameters_clean_older_than(time_t t)
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   return _parsed_parameters_clean_cache(t, 0);
#endif
}


int16_t parsed_parameters_init()
{
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   if(_parsed_parameters_cache_rwlock==NULL) {
      _parsed_parameters_cache_rwlock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
      if(!_parsed_parameters_cache_rwlock) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return -1;
      }
      pthread_rwlock_init(_parsed_parameters_cache_rwlock, NULL);
   }
#endif
   return 0;
}


char *getToken(char *str)
{
   char *end=NULL;
   
   // suppression des blancs avant
   while(isspace(*str) && str)
      str++;
   
   if(*str!=0) { // si la chaine n'est pas vide
      end=str+strlen(str) - 1;
      
      // suppression des blancs après
      while(end > str && isspace(*end))
         end--;
      *(end+1)=0;
   }
   else
      return NULL;
   
   // vérification des caractères
   for(int i=0;str[i];i++) {
      if(!(isalpha(str[i]) || isdigit(str[i]) || str[i]=='_' || str[i]=='.' || str[i]==':'))
         return NULL;
      
      str[i]=toupper(str[i]);
   }
   
   return str;
}


static void _clean_parsed_parameters(parsed_parameters_t *params)
{
   for(int i=0;i<params->nb;i++) {
      if(params->parameters[i].type==STRING) {
         if(params->parameters[i].value.s) {
            free(params->parameters[i].value.s);
            params->parameters[i].value.s=NULL;
         }
      }
   }
}


void release_parsed_parameters(parsed_parameters_t **params)
{
   if(*params) {
      (*params)->in_use--;

#ifdef PARSED_PARAMETERS_CACHING_ENABLED
      if((*params)->from_cache==1) {
         *params=NULL;
         return;
      }
#endif

      if((*params)->in_use==0) {
         _clean_parsed_parameters(*params);

         free(*params);
      }
      *params=NULL;
   }
}


parsed_parameters_t *alloc_parsed_parameters(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper)
{
/*
 parsed_parameter_t *mpp=malloc_parsed_parameters("aA=1; b = 00021;C=10;;X=1;A_a =20.0;", tofind, &nb);
 */
   char *ptr = parameters_string;
   char label[21];
   char *label_token;
   char *value;
   char *value_token;
   int n;
   int ret;
   parsed_parameters_t *parsed_parameters;
   
   if(err)
      *err=0;
      
   if(parameters_string == NULL) {
      if(err)
         *err=11;
      return NULL;
   }
   
   if(strlen(parameters_string)==0) {
      if(err)
         *err=10;
      return NULL;
   }
   
   // nombre max de label dans la liste
   for(*nb_params=0;parameters_to_find[*nb_params];(*nb_params)++); // nombre max de parametres
   
   // préparation du tableau à retourner
   if(!*nb_params) {
      // afficher ici un message d'erreur
      return NULL;
   }

#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   parsed_parameters=_parsed_parameters_get_from_cache(parameters_to_find, *nb_params, parameters_string);
   if(parsed_parameters) {
      parsed_parameters->in_use++;
      return parsed_parameters;
   }
#endif

   value=malloc(strlen(parameters_string+1)); // taille de valeur au max (et même un peu plus).
   if(!value) {
      if(err)
         *err=1;
      return NULL;
   }
   
   parsed_parameters=malloc(sizeof(parsed_parameters_t));
   if(!parsed_parameters) {
      DEBUG_SECTION {
         fprintf (MEA_STDERR, "%s (%s) : %s - ",DEBUG_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      if(err)
         *err=1; // erreur système, voir errno
      return NULL;
   }
 

   parsed_parameters->parameters=malloc(sizeof(parsed_parameter_t) * (*nb_params));
   if(!parsed_parameters->parameters) {
      DEBUG_SECTION {
         fprintf (MEA_STDERR, "%s (%s) : %s - ",DEBUG_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      free(parsed_parameters);
      parsed_parameters=NULL;
      if(err)
         *err=1; // erreur système, voir errno
      return NULL;
   }
 
   memset(parsed_parameters->parameters, 0, sizeof(parsed_parameter_t) * *nb_params);
   parsed_parameters->nb=*nb_params;
 
   while(1) {
      ret=sscanf(ptr, "%20[^=;]=%[^;]%n", label, value, &n); // /!\ pas plus de 20 caractères pour un TOKEN
      if(ret==EOF) // plus rien à lire
         break;
      
      if(ret==2) {
         // ici on traite les données lues
         label_token=getToken(label);
         if(value_to_upper)
            mea_strtoupper(value);
         value_token=value;
         
         char trouvee=0;
         for(int i=0;parameters_to_find[i];i++) {
            char *str=&(parameters_to_find[i][2]);
            if(label_token && strcmp(label_token,str)==0) {
               char type=parameters_to_find[i][0];
               trouvee=1;
               parsed_parameters->parameters[i].label=str;
               int r=-1;
               switch(type) {
                  case 'I':
                     parsed_parameters->parameters[i].type=INT;
                     sscanf(value_token,"%d%n",&(parsed_parameters->parameters[i].value.i),&r);
                     break;
                     
                  case 'L':
                     parsed_parameters->parameters[i].type=LONG;
                     sscanf(value_token,"%ld%n",&(parsed_parameters->parameters[i].value.l),&r);
                     break;
                     
                  case 'F':
                     parsed_parameters->parameters[i].type=FLOAT;
                     sscanf(value_token,"%f%n",&(parsed_parameters->parameters[i].value.f),&r);
                     break;
                     
                  case 'S':
                     parsed_parameters->parameters[i].type=STRING;
                     r=(int)strlen(value_token);
                     parsed_parameters->parameters[i].value.s=malloc(r+1);
                     if(!parsed_parameters->parameters[i].value.s) {
                        DEBUG_SECTION {
                           fprintf (MEA_STDERR, "%s (%s) : %s - ", DEBUG_STR, __func__, MALLOC_ERROR_STR);
                           perror("");
                        }
                        if(parsed_parameters) {
                           _clean_parsed_parameters(parsed_parameters);
                           free(parsed_parameters);
                           parsed_parameters=NULL;
                        }
                        if(err)
                           *err=1; // erreur système, voir errno
                        goto malloc_parsed_parameters_exit;
                     }
                     strncpy(parsed_parameters->parameters[i].value.s, value_token,r);
                     parsed_parameters->parameters[i].value.s[r]=0;
                     break;
                     
                  default:
                     break;
               }
               if(r!=strlen(value_token)) {
                  if(parsed_parameters) {
                     _clean_parsed_parameters(parsed_parameters);
                     free(parsed_parameters);
                     parsed_parameters=NULL;
                  }
                  if(err)
                     *err=2; // erreur de syntaxe;
                  goto malloc_parsed_parameters_exit;
               }
               break;
            }
         }
         if(!trouvee) {
            if(parsed_parameters) {
               _clean_parsed_parameters(parsed_parameters);
               free(parsed_parameters);
               parsed_parameters=NULL;
            }
            if(err)
               *err=3; // label inconnu
            goto malloc_parsed_parameters_exit;
         }
         // déplacement du pointeur sur les données suivantes
         ptr+=n;
      }
      
      if(*ptr == ';') { // séparateur, on passe au caractère suivant
         ptr++;
         if(!ret) // si on avait rien lu
            ret=2; // on fait comme si on avait lu ...
      }
      
      if(*ptr == 0) // fin de ligne, OK.
         break; // sortie de la boucle

      if(ret<2) { // si on a pas un label et une valeur
         if(parsed_parameters) {
            _clean_parsed_parameters(parsed_parameters);
            free(parsed_parameters);
            parsed_parameters=NULL;
         }
         if(err)
            *err=4; // label sans valeur
         goto malloc_parsed_parameters_exit;
      }
   }
   free(value);
   value=NULL;

#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   _parsed_parameters_add_to_cache(parameters_to_find, parameters_string, parsed_parameters);
#endif
   parsed_parameters->in_use=1;
   return parsed_parameters;

malloc_parsed_parameters_exit:
   if(value) {
      free(value);
      value=NULL;
   }
   return NULL;
}


void display_parsed_parameters(parsed_parameters_t *mpp)
{
   DEBUG_SECTION {
      if(mpp) {
         for(int i=0;i<mpp->nb;i++) {
            printf("%s = ", mpp->parameters[i].label);
            switch((int)(mpp->parameters[i].type)) {
               case 1:
                  printf("%d (I)\n",mpp->parameters[i].value.i);
                  break;
               case 2:
                  printf("%ld (L)\n",mpp->parameters[i].value.l);
                  break;
               case 3:
                  printf("%f (F)\n",mpp->parameters[i].value.f);
                  break;
               case 4:
                  printf("%s (S)\n",mpp->parameters[i].value.s);
                  break;
               default:
                  printf("\n");
                  break;
            }
         }
      }
   }
}


#ifdef PARAMETERS_UTILS_MODULE_R7
// gcc -std=c99 -std=gnu99 parameters_utils.c mea_string_utils.c -lpthread -DPARAMETERS_UTILS_MODULE_R7
#include <sys/time.h>

double millis()
{
   struct timeval te;
   gettimeofday(&te, NULL);

   double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0;

   return milliseconds;
}

char *params1_str[]={"S:P1","S:P2",NULL};
#define PARAMS1_1 0
#define PARAMS1_2 1
char *params2_str[]={"S:P3","S:P4","S:P5",NULL};
#define PARAMS2_1 0
#define PARAMS2_2 1
#define PARAMS2_3 2

int main(int argc, char *argv[])
{
   int nb_params=0;
   char *test1_str="P2=1;P1=2";
   char *test2_str="P2=3;P1=4";
   char *test3_str="P3=5;P4=5";
   int err;

   parsed_parameters_t *params1_1,*params1_2;
   parsed_parameters_t *params2;

   parsed_parameters_init();

   params1_1=alloc_parsed_parameters(test1_str, params1_str, &nb_params, &err, 0);   
   for(int i=0;i<nb_params;i++)
      fprintf(stderr,"%s = %s\n",params1_1->parameters[i].label, params1_1->parameters[i].value.s);
   release_parsed_parameters(&params1_1);

   double t0=millis();
   for(int i=0;i<1000000;i++) {
      params1_2=alloc_parsed_parameters(test2_str, params1_str, &nb_params, &err, 0);   
      release_parsed_parameters(&params1_2);
   }
   fprintf(stderr, "%5.2f ms\n",millis()-t0);

   parsed_parameters_clean_older_than(1);

   fprintf(stderr, "=====\n");

   params2=alloc_parsed_parameters(test3_str, params2_str, &nb_params, &err, 0); 
//   release_parsed_parameters(&params2);

   parsed_parameters_clean_all(0);

   fprintf(stderr, "=====\n");

   parsed_parameters_clean_all(1);
}
#endif
