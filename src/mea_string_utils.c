//
//  mea_string_utils.c
//
//  Created by Patrice DIETSCH on 08/07/2013.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "mea_string_utils.h"


void mea_strcpylower(char *d, char *s)
{
   while(*s) {
      *d=tolower(*s);
      ++d;
      ++s;
   }
   *d=0;
}


void mea_strncpylower(char *d, char *s, int n)
{
   if(n<0)
      return;
   while(*s && n) {
      *d=tolower(*s);
      --n;
      ++d;
      ++s;
   }
   if(n)
      *d=0;
}


void mea_strcpytrim(char *d, char *s)
{
   char *p=d;

   while(*s && *s==' ') ++s;

   while(*s) {
      *d=*s;
      ++d;
      if(*s!=' ')
         p=d;
      ++s;
   }
   *p=0;
}


void mea_strcpytrimlower(char *d, char *s)
{
   char *p=d;

   while(*s && *s==' ') ++s;

   while(*s) {
      *d=tolower(*s);
      ++d;
      if(*s!=' ')
         p=d;
      ++s;
   }
   *p=0;
}


void mea_strncpytrim(char *d, char *s, int n)
{
   if(n<0)
      return;

   char *p=d;
   while(*s && *s==' ') ++s;

   while(*s && n) {
      *d=*s;
      --n;
      ++d;
      if(*s!=' ')
         p=d;
      ++s;
   }
   if(n)
      *p=0;
}


void mea_strncpytrimlower(char *d, char *s, int n)
{
   if(n<0)
      return;

   char *p=d;
   while(*s && *s==' ') ++s;

   while(*s && n) {
      *d=tolower(*s);
      --n;
      ++d;
      if(*s!=' ')
         p=d;
      ++s;
   }
   if(n)
      *p=0;
}


char *mea_strltrim(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche d'une chaine.
  * \details   la chaine n'est pas modifiée. un pointeur est retourné sur le
  *            premier caractère "non blanc"
  *            de la chaine. Attention à ne pas perdre le pointeur d'origine
  *             pour les chaines allouées.
  *            Eviter 'absolument s=mea_strltrim(s)', préférez 's_trimed = 
  *            strltrim(s)'.
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur le premier caractère "non blanc" de la chaine
  */
{
   if(!s)
      return NULL;

   while(*s && isspace(*s)) s++;
   return s;
}


char *mea_strltrim2(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche d'une chaine.
  * \details   la chaine est modifiée (décallage de tous les caractères non
  *            blanc vers le début de la chaine)
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur la chaine ou NULL si (s=NULL)
  */
{
   if(!s)
      return NULL;

   if(!*s)
      return s;

   char *p=s;

   while(*s && isspace(*s)) s++;

   int i;
   for(i=0;*s;i++,s++)
      p[i]=*s;
   p[i]=0;
   
   return p;
}


char *mea_strltrim_alloc(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche d'une chaine.
  * \details   la chaine est recopiée dans une nouvelle zone mémoire
  *            (il faudra la libérer par free).
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur la nouvelle chaine ou NULL (erreur d'allocation)
  */
{
   char *s_trimed = NULL;
   
   if(!s)
      return NULL;

   s_trimed=mea_strltrim(s);
   
   s_trimed=mea_string_alloc_and_copy(s_trimed);
   if(!s_trimed) {
      return NULL;
   }
   else
      return s_trimed;
}


char *mea_strrtrim(char *s)
 /**
  * \brief     trim (suppression de blanc) à droite d'une chaine.
  * \details   la chaine est modifiée. un '\0' est positionné à la place du
  *            premier "blanc" des blancs en fin de chaine.
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur le début de la chaine ou NULL (si s=NULL)
  */
{
   if(!s)
      return NULL;

   if(!*s) // chaine vide
      return s;

   char* back = s + strlen(s);
   while(isspace(*--back));
   *(back+1) = '\0';
   return s;
}


char *mea_strrtrim_alloc(char *s)
 /**
  * \brief     trim (suppression de blanc) à droite d'une chaine.
  * \details   la chaine est recopiée dans une nouvelle zone mémoire (il faudra
  *            la libérer par free).
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur la nouvelle chaine ou NULL erreur d'allocation ou si s=NULL
  */
{
   char *s_trimed;
   char *s_new;
   
   if(!s)
      return NULL;

   s_trimed=mea_string_alloc_and_copy(s);
   if(!s_trimed) {
      return NULL;
   }
   else {
      mea_strrtrim(s_trimed);
      s_new=(char *)realloc(s_trimed, strlen(s_trimed)+1);

      if(s_new)
         return s_new;
      else {
         free(s_trimed);
         s_trimed=NULL;
         return NULL;
      }
   }
}


char *mea_strtrim2(char *s)
{
    return mea_strrtrim(mea_strltrim2(s));
}


char *mea_strtrim(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche et à droite d'une chaine.
  * \details   la chaine est modifiée, un '\0' un '\0' est positionné à la place
  *            du premier "blanc"
  *            des blancs en fin de chaine (voir mea_strrtrim()) et un pointeur
  *            est retourné sur le premier caractère "non blanc" de la chaine
  *            (voir mea_strltrim())
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur le début de la chaine.
  */
{
    return mea_strrtrim(mea_strltrim(s));
}


char *mea_strtrim_alloc(char *s)
 /**
  * \brief     trim (suppression de blanc) à gauche et à droite d'une chaine.
  * \details   la chaine n'est pas modifiée, une nouvelle chaine est allouée (à
  *            supprimer ensuite par free).
  *
  * \param     s  chaine à trimer
  *
  * \return    pointeur sur la nouvelle chaine ou NULL (erreur d'allocation)
  */
{
   char *s_trimed;
   
   if(!s)
      return NULL;

   s_trimed=mea_strltrim(s);
   if(s_trimed) {
      s_trimed=mea_strrtrim_alloc(s_trimed);
      if(s_trimed)
         return s_trimed;
      else {
         return NULL;
      }
   }
   else
      return NULL;
}


void mea_strtoupper(char *str)
/**
 * \brief     convertit tous les caractères d'une chaine en majuscule
 * \details   La chaine en paramètre est modifiée : les minuscules sont
 *            remplacées par des majuscules.
 *
 * \param     str  chaine à modifier.
 *
 * \return    pas de retour.
 */
{
   if(!str)
      return;
   uint16_t i=0;   
   for(;i<strlen(str);i++)
      str[i]=toupper(str[i]);
}


void mea_strtolower(char *str)
/**
 * \brief     convertit tous les caractères d'une chaine en minuscules
 * \details   La chaine en paramètre est modifiée : les majuscules sont
 *            remplacées par des minuscules.
 *
 * \param     str  chaine à modifier.
 *
 * \return    pas de retour.
 */
{
   if(!str)
      return;

   uint16_t i=0;
   for(;i<strlen(str);i++)
      str[i]=tolower(str[i]);
}


int16_t mea_strcmplower(char *str1, char *str2)
/**
 * \brief     comparaison de deux chaines sur la base de "caractères en
 *            mimuscules"
 * \details   chaque caractère des deux chaines est converti en minuscule avant
 *            de les comparer
 *
 * \param     str1   premiere chaine à comparer.
 * \param     str2   deuxième chaine à comparer.
 *
 * \return    0 chaines égales, 1 chaines différentes
 */
{
   if(!str1 || !str2)
      return -1;
   
   int i=0;
   for(;str1[i] && str2[i];i++) {
      if(tolower(str1[i]) - tolower(str2[i])!=0)
         break;
   }
   return tolower(str1[i]) - tolower(str2[i]);
}


int16_t mea_strcmplower2(char *str1, char *str2)
/**
 * \brief     comparaison de deux chaines sur la base de "caractères en
 *            mimuscules"
 * \details   chaque caractère des deux chaines est converti en minuscule avant
 *            de les comparer
 *
 * \param     str1   premiere chaine à comparer.
 * \param     str2   deuxième chaine à comparer.
 *
 * \return    0 chaines égales, 1 chaines différentes
 */
{
   int i;

   if(!str1 || !str2)
      return 0;

   for(i=0;str1[i];i++) {
      if(tolower(str1[i])<tolower(str2[i]))
         return -1;
      else if(tolower(str1[i])>tolower(str2[i]))
         return 1;
   }

   if(str1[i] && !str2[i]) {
      return 1;
   }
   else if(!str1[i] && str2[i]) {
      return -1;
   }
   return 0;
}


int16_t mea_strncmplower(char *str1, char *str2, int n)
/**
 * \brief     comparaison des n premiers caracteres de deux chaines sur la base
 *            de "caractères en mimuscules"
 * \details   chaque caractère des deux chaines est converti en minuscule avant
 *            comparaison
 *
 * \param     str1   premiere chaine à comparer.
 * \param     str2   deuxième chaine à comparer.
 * \param     n      nombre max de caracteres à comparer
 *
 * \return    0 chaines égales, 1 chaines différentes. Si n<0 les chaines sont
 *            considérées comme égale et 0 est retourné
 */
{
   int i;
   if(n<1)
      return 0;
   
   if(!str1 || !str2)
      return 0;

   for(i=0;str1[i] && i<n;i++) {
      if(tolower(str1[i])!=tolower(str2[i]))
         return 1;
   }
   if(i==n || str1[i]==str2[i])
      return 0;
   else
      return 1;
}


int16_t mea_strsplit(char str[], char separator, char *tokens[], uint16_t l_tokens)
/**
 * \brief     découpe une zone mémoire représentant une chaine en "n" chaines en
 *            remplaçant le "séparateur" par un "\0" dans la chaine d'origine
 * \details   la zone mémoire est concervée en l'état.
 *
 * \param     str        chaine à découper.
 * \param     separator  le séparateur entre chaque "token"
 * \param     tokens     table de pointeur de chaine (alloué par l'appelant)
 *                       qui contiendra le pointeur sur les chaines
 * \param     l_tokens   la taille maximum (nombre d'éléments) que peut contenir
 *                       la table de pointeur
 *
 * \return    -1 en cas d'erreur (taille de tokens insuffisante), nombre de
 *             tokens trouvé.
 */
{
   int j=0;
   tokens[j]=str;
   int i=0;
   for(;str[i];i++)
   {
      if(str[i]==separator) {
         str[i]=0;
         j++;
         if(j<l_tokens)
            tokens[j]=&(str[i+1]);
         else
            return -1;
      }
   }
   return j+1;
}


size_t mea_snprintfcat(char* buf, size_t bufSize, char const* fmt, ...)
/**
 * \brief     fait un "snprintf" en ajoutant le résultat à la fin d'une chaine
 * \details   voir snprintf pour le format
 * \param     buf        pointeur sur la chaine à compléter.
 * \param     bufSize    taille max que peut contenir la chaine
 * \param     fmt        nombre de paramètre variable ... (voir snprintf)
 * \return    taille de la nouvelle chaine.
 */
{
   size_t result;
   va_list args;
   size_t len = strnlen(buf, bufSize);

   va_start(args, fmt);
   
   result = vsnprintf(buf + len, bufSize - len, fmt, args);
   
   va_end(args);

   return result + len;
}


int mea_strncat(char *dest, int max_test, char *source)
/**
 * \brief     fait un "strcat" avec limitation de la taille de la chaine
 *            résultante
 *
 * \param     dest       pointeur sur la chaine à compléter.
 * \param     bufSize    taille max que peut contenir la chaine à complété
 * \param     source     pointeur sur la chaine à rajouter à dest
 *
 * \return    -1 si la taille de dest ne peut pas contenir source (dest n'est
 *            pas modifié), 0 sinon
 */
{
   int l_dest = (int)strlen(dest);
   int l_source = (int)strlen(source);
   if((l_dest+l_source)>max_test) {
      return -1;
   }
   else {
      strcat(dest, source);
   }
   return 0;
}


/**
 * \brief     supprime tous les caractères "blancs" (espace)
 *            d'une chaine
 *
 * \param     str        pointeur sur la chaine à traiter.
 *
 */
void mea_strremovespaces(char *str)
{
   char c, *p;

   p = str;
   do {
      while ((c = *p++) == ' ');
   }
   while ((*str++ = c) != 0);

   return;
}


char *mea_string_alloc_and_copy(char *str)
/**
 * \brief     crée une copie d'une chaine dans une nouvelle zone allouée.
 * 
 * \param     str       chaine à copier.
 *
 * \return    pointeur sur une nouvelle zone mémoire contenant la chaine ou
 *            NULL si la nouvelle zone n'a pas pu être allouée
 */
{
   char *new_str=(char *)malloc(strlen(str)+1);
   if(new_str) {
      strcpy(new_str, str);
   }
   else {  
      return NULL;
   }
   return new_str;
}


char *mea_string_free_alloc_and_copy(char **org_str, char *str)
/**
 * \brief     libère une zone mémoire et réaloue une nouvelle zone pour y
 *            copier str.
 *
 * \param     org_str   pointeur sur le pointeur de la chaine à libérer et
 *                      réalouer.
 * \param     str       chaine à copier.
 *
 * \return    pointeur sur la nouvelle chaine.
 */
{
   if(*org_str) {
      free(*org_str);
      *org_str=NULL;
   }
   
   *org_str=(char *)malloc(strlen(str)+1);
   if(*org_str)
      strcpy(*org_str, str);
   else {
      return NULL;
   }
   return *org_str;
}


int mea_strisnumeric(char *str)
{
   int i=0;
   for(;str[i];i++) {
      if(!isdigit(str[i]))
         return -1;
   }
   return 0;
}


int mea_strcpy_escs(char *dest, char *source)
{
   int i=0,j=0;
   for(;source[i];i++,j++) {
      char c=source[i];
      if(c=='\\') {
         i++;
         c=source[i];
         switch(c) {
            case '\\':
            case '"':
            case '#':
               break;
            case 'n':
               c=10;
               break;
            case 'r':
               c=13;
               break;
            default:
               dest[0]=0;
               return -1;
         }
      }
      dest[j]=c;
   }
   dest[j]=0;

   return 0;
}


int mea_strcpy_escd(char *dest, char *source)
{
   int i=0, j=0;
   for(;source[i];i++,j++) {
      char c=source[i];

      switch(c) {
         case '\\':
         case '"':
         case '#':
            dest[j++]='\\';
            break;
         case 10:
            dest[j++]='\\';
            c='n';
            break;
         case 13:
            dest[j++]='\\';
            c='r';
            break;
      }

      dest[j]=c;
   }
   dest[j]=0;

   return 0;
}


int mea_strlen_escaped(char *s)
{
   int i=0,j=0;

   for(;s[i];i++) {
      switch(s[i]) {
         case '\\':
         case '"':
         case 10:
         case 13:
         case '#':
            j++;
            break;
      }
      j++;
   }

   return j;
}


#define ISOCTAL(c)	(isdigit(c) && (c) != '8' && (c) != '9')

char *mea_unescape(char *result, char *data)
{
   int ch;
   int result_ptr = 0;
   int oval;
   int i;

   result[0]=0;

   while ((ch = *data++) != 0) {
      if (ch == '\\') {
	      if ((ch = *data++) == 0)
		      break;
	         switch (ch) {
	            case 'a':				/* \a -> audible bell */
		            ch = '\a';
		            break;
	            case 'b':				/* \b -> backspace */
		            ch = '\b';
		            break;
	            case 'f':				/* \f -> formfeed */
		            ch = '\f';
		            break;
	            case 'n':				/* \n -> newline */
		            ch = '\n';
		            break;
	            case 'r':				/* \r -> carriagereturn */
		            ch = '\r';
		            break;
	            case 't':				/* \t -> horizontal tab */
		            ch = '\t';
		            break;
	            case 'v':				/* \v -> vertical tab */
		            ch = '\v';
		            break;
	            case '0':				/* \nnn -> ASCII value */
	            case '1':
	            case '2':
	            case '3':
	            case '4':
	            case '5':
	            case '6':
	            case '7':
		            for (oval = ch - '0', i = 0; i < 2 && ((ch = *data) != 0) && ISOCTAL(ch); i++, data++) {
		               oval = (oval << 3) | (ch - '0');
		            }
		            ch = oval;
		            break;
	            default:				/* \any -> any */
		            break;
	       }
	   }
      result[result_ptr++]=ch;
   }
   result[result_ptr]=0;
   return (result);
}

#ifdef MEA_STRING_UTILS_MODULE_TEST
int main(int argc, char *argv[])
{
   char s1[80]="   ABCDEFG   ";
   char s2[80]="   HIJKLMN   ";
   char s3[80]="\\\"TO\\\\TO\\\"";

   char *str1, *str2, *str1bis, *str2bis = NULL;

   str1bis=mea_strtrim_alloc(s1);
   if(str1bis)
      fprintf(stderr,"str1bis=\"%s\"\n",str1bis);
   else
      fprintf(stderr,"str1bis=NULL\n");
   free(str1bis);

   str1=mea_strltrim2(s1);
   if(str1)
      fprintf(stderr,"str1=\"%s\"\n",str1);
   else
      fprintf(stderr,"str1=NULL\n");

   str2=mea_strltrim_alloc(s2);
   if(str2)
      fprintf(stderr,"str2=\"%s\"\n",str2);
   else
      fprintf(stderr,"str2=NULL\n");

   str2bis = mea_strrtrim_alloc(s2);
   if(str2bis)
      fprintf(stderr,"str2bis=\"%s\"\n",str2bis);
   else
      fprintf(stderr,"str2bis=NULL\n");

   char d1[80],d2[80];

   mea_strcpy_escs(d1,s3);
   fprintf(stderr,"escs=%s (%s)\n",d1, s3);

   mea_strcpy_escd(d2,d1);
   fprintf(stderr,"escs=%s (%s)\n",d2, d1);

   fprintf(stderr,"mea_strlen_escaped=%d\n", mea_strlen_escaped(d1));

   free(str2);
   free(str2bis);

   char s4[80];

   mea_strcpytrim(s4, "A");
   fprintf(stderr,"s4 = [%s]\n", s4);

   mea_strncpytrim(s4, s2, 15);
   fprintf(stderr,"s4 = [%s]\n", s4);

   mea_unescape(s4, "\\101\\102\\n");
   fprintf(stderr,"S4=%s\n", s4);

}
#endif
