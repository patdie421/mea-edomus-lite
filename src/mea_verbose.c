#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/file.h>

#include "mea_verbose.h"

int verbose_level=10;
int debug_msg=1;

const char *_error_str = "ERROR";
const char *_info_str  = "INFO ";
const char *_debug_str = "DEBUG";
const char *_fatal_error_str="FATAL ERROR";
const char *_warning_str="WARNING";
const char *_malloc_error_str="malloc error";


void set_verbose_level(int level)
/**
 * \brief     positionne le niveau de logging.
 * \details   le niveau n'est pas gérer directement (pas de min et pas de max). C'est au programmeur d'en faire la gestion.
 * \param     level   valeur à positionner.
 */
{
   verbose_level=level;
}


void debug_on()
/**
 * \brief     active le mode message de debug
 */
{
   debug_msg=1;
}


void debug_off()
{
/**
 * \brief     désactive le mode debug message de debug
 */
   debug_msg=0;
}


int16_t debug_status()
/**
 * \brief     return l'état du mode de message de debug
 * \return    0 debug desactivé, 1 debug activé
 */
{
   return debug_msg;
}


int mea_rotate_open_log_file(char *name, uint16_t max_index)
{
   int ret_code=0;
   int ret=0;
   char *name_old=NULL, *name_new=NULL;
   FILE *fd=NULL;

   fd=fopen(name, "r+");
   if(!fd)
       return -1;

   name_old = (char *)malloc(strlen(name)+7); // 7 => '.' + 5 digits + 1
   if(name_old == NULL) {
      ret_code=-1;
      goto mea_rotate_open_log_file_clean_exit;
   }

   // removing oldest file if needed
   sprintf(name_old,"%s.%d", name, max_index);
   if(access( name_old, W_OK ) != -1) {
      ret=unlink(name_old);
      if(ret<0) {
         perror("unlink: ");
         ret_code=-1;
         goto mea_rotate_open_log_file_clean_exit;
      }
   }


   name_new = (char *)malloc(strlen(name)+7); // 7 => '.' + 5 digits + 1
   if(name_new == NULL) {
      ret_code=-1;
      goto mea_rotate_open_log_file_clean_exit;
   }

   int16_t i=max_index-1;
   for(;i>-1;i--) {
      sprintf(name_new,"%s.%d", name, i+1);
      sprintf(name_old,"%s.%d", name, i);

      if(access( name_old, W_OK ) != -1) {
         ret=rename(name_old, name_new);
         if(ret<0) {
            perror("rename: ");
            ret_code=-1;
            goto mea_rotate_open_log_file_clean_exit;
         }
      }
   }

   sprintf(name_new,"%s.%d", name, 0);
   int fd_dest=open(name_new, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
   if(fd_dest<0) {
      perror("open: ");
      ret_code=-1;
      goto mea_rotate_open_log_file_clean_exit;
   }
 
   flock(fileno(fd), LOCK_EX);

   fseek(fd, 0, SEEK_SET);
   // size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
   char buf[4096];
   while(!feof(fd)) {
      size_t nb=fread(buf, 1, sizeof(buf), fd);
      if(write(fd_dest, buf, nb)==-1) {
         perror("write: ");
      }
   }
   if(!ftruncate(fileno(fd), 0)) {
      perror("ftruncate: ");
   } 

   flock(fileno(fd), LOCK_UN);

   close(fd_dest);
   fclose(fd);
 
mea_rotate_open_log_file_clean_exit:
   if(fd)
      fclose(fd);
   if(name_old) {
      free(name_old);
      name_old=NULL;
   }
   if(name_new) {
      free(name_new);
      name_new=NULL;
   }

   return ret_code;
}

 
void mea_log_printf(char const* fmt, ...)
/**
 * \brief     imprime un message de type log
 * \details   un "timestamp" est afficher avant le message. Pour le reste s'utilise comme printf.
 * \param     _last_time   valeur retournée par start_chrono
 * \return    différence entre "maintenant" et _last_chrono en milliseconde
 */
{
   va_list args;
   static char *date_format="[%Y-%m-%d %H:%M:%S]";

   char date_str[40];
   time_t t;
   struct tm t_tm;

   t=time(NULL);

   if (localtime_r(&t, &t_tm) == NULL)
      return;

   if (strftime(date_str, sizeof(date_str), date_format, &t_tm) == 0)
      return;

   va_start(args, fmt);

   flock(fileno(MEA_STDERR), LOCK_EX);

   fprintf(MEA_STDERR, "%s ", date_str);
   vfprintf(MEA_STDERR, fmt, args);

   flock(fileno(MEA_STDERR), LOCK_UN);

   va_end(args);
}

#ifdef MODULE_R7
int main(int argc, char *argv[])
{
   mea_rotate_open_log_file((FILE *)NULL, "test.log", 5);
}
#endif
