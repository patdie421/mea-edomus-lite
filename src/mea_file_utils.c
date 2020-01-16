#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>


#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <sys/sendfile.h>
#endif

#include "mea_file_utils.h"


int mea_filebackup(char *filename)
{
   char _time[256]="";
   char filenamebak[1024]="";
   
   time_t timestamp = time(NULL); 
   
   strftime(_time, sizeof(_time)-1, "%Y-%m-%d_%X", localtime(&timestamp));
   strcpy(filenamebak, filename);
   strcat(filenamebak, ".");
   strcat(filenamebak, _time);

   return mea_filecopy(filename, filenamebak);
}


int mea_filecopy(const char* source, const char* destination)
{
   int input=-1, output=-1;
   
   if ((input = open(source, O_RDONLY)) == -1) {
      return -1;
   }
   if ((output = creat(destination, 0660)) == -1) {
      close(input);
      return -1;
  }

   //Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
    //fcopyfile works on FreeBSD and OS X 10.5+ 
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    //sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
   off_t bytesCopied = 0;
   struct stat fileinfo = {0};
   fstat(input, &fileinfo);
   int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

   close(input);
   close(output);

   if(result==-1) {
      return 1;
   }
   else {
      return 0;
   }
}
