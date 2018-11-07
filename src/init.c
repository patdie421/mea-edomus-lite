/**
 * \file      init.c
 * \author    patdie421
 * \version   0.1 * \date      30 octobre 2013
 * \brief     Ensemble des fonctions qui permettent la gestion de l'initialisation et la mise à jour de l'application.
 * \details   initialisation automatique, initialisation interactive et mise à jour des paramètres de l'application.
 */ 
//
//  init.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sqlite3.h>

#include "init.h"

#include "globals.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "sqlite3db_utils.h"
#include "interfacesServer.h"
#include "cJSON.h"
#include "configuration.h"
#include "mea_json_utils.h"

char const *usr_str="/usr";

struct types_value_s
{
   int16_t  id_type;
   char *name;
   char *description;
   char *parameters;
   char *flag;
   char *typeoftype;
}; 


int16_t linesToFile(char *file, char *lines[])
 /**
  * \brief     Création d'un fichier (texte) avec les lignes passées en paramètres (tableau de chaine).
  * \details   Le dernier élément du tableau de lignes doit être NULL.
  * \param     file    nom et chemin (complet) du fichier à créer.
  * \param     lines   tableau de pointeur sur les chaines à intégrer au fichier.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur
  */
{
   FILE *fd;
   
   fd=fopen(file,"w");
   if(fd) {
      int error_flag=0;
      for(int16_t i=0;lines[i];i++) {
         if(fprintf(fd,"%s\n",lines[i])<0) {
            error_flag=-1;
            break;
         }
      }
      fclose(fd);
      
      return error_flag;
   }
   else
      return -1;
}


char *get_and_malloc_string(char *default_value, char *question_str)
 /**
  * \brief     Demande une chaine de caractères depuis l'entrée standard et retourne la valeur dans une chaine allouée (malloc).
  * \details   Une question est affichée et une propostion de valeur est faite (validée par ENTER sans saisir de caractères pour accepter la proposition).
  * \param     default_value  valeur proposée par défaut
  * \param     question_str   la question à poser
  * \return    une chaine qui a été allouée ou NULL en cas d'erreur (voir aussi errno)
  */
{
   char read_str[1024];
   char *read_str_trimed;
   char *to_return_str=NULL;

   if(!default_value)
      default_value="";

   do {
      printf("%s [%s] : ", question_str, default_value);
      if(fgets(read_str,sizeof(read_str), stdin)) {
         read_str[strlen(read_str)-1]=0; // retire le \n à la fin de la ligne
         read_str_trimed=mea_strtrim(read_str); // retire les blanc en début et fin de chaine
         if(!strlen(read_str_trimed)) { // appuie sur Enter sans saisir de caractères
            strcpy(read_str, default_value); // on prend la valeur qu'on a prosée
            read_str_trimed=read_str;
         }
      }
      else {
         VERBOSE(9) {
            mea_log_printf ("%s (%s) : fgets - ", DEBUG_STR,__func__);
            perror("");
         }
         return NULL; // en cas d'erreur sur fgets
      }
   } while (strlen(read_str_trimed)==0);
    
   to_return_str=(char *)malloc(strlen(read_str_trimed)+1);
   strcpy(to_return_str, read_str_trimed);
    
   return to_return_str;
}


char *get_and_malloc_path(char *base_path, char *dir_name, char *question_str)
 /**
  * \brief     Demande la saisie d'un chemin de fichier ou de répertoire depuis l'entrée standard et retourne la valeur dans une chaine allouée (malloc).
  * \details   Une question est affichée et une propostion de répertoire est faite (validée par ENTER sans saisir de caractères).
  * \param     base_path    chemin de base (basename) pour la proposition de fichier/répertoire  (ie : première partie du chemin)
  * \param     dir_name     nom (ou fin de chemin) du fichier/répertoire "final" (ie : deuxième partie du chemin)
  * \param     question_str la question à poser
  * \return    le chemain complet dans une chaine qui a été allouée ou NULL en cas d'erreur
  */
{
   char proposed_path_str[1024];

   if(base_path) {
      int16_t n=snprintf(proposed_path_str,sizeof(proposed_path_str), "%s/%s", base_path, dir_name);
      if(n<0 || n==sizeof(proposed_path_str)) {
         VERBOSE(9) {
            mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
            perror("");
         }
         return NULL;
      }
   }
   else // si pas de base path, pas de proposition à faire
      proposed_path_str[0]=0;
      
   return get_and_malloc_string(proposed_path_str, question_str);
}

   
char *get_and_malloc_integer(int16_t default_value, char *question_str)
 /**
  * \brief     Demande un entier depuis l'entrée standard et retourne la valeur sous forme d'une chaine allouée (malloc).
  * \details   Une question est affichée et une propostion de valeur par défaut est faite (validée par ENTER sans saisir de caractères).
  * \param     default_value  valeur proposée
  * \param     question_str   la question à poser
  * \return    une chaine qui a été allouée ou NULL en cas d'erreur
  */
{
   char *value_str;
   char *end;
   char default_value_str[16];
   
   int16_t n=snprintf(default_value_str,sizeof(default_value_str),"%d",default_value);
   if(n<0 || n==sizeof(default_value_str)) {
      VERBOSE(9) {
         mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
         perror("");
      }
      return NULL;
   }

   while(1) {
      value_str=get_and_malloc_string(default_value_str, question_str);
      if(!value_str)
         return NULL;
      strtol(value_str,&end,10);
      if(*end!=0 || errno==ERANGE) {
         free(value_str);
         value_str=NULL;
      }
      else
         break;
   }
   
   return value_str;
}


int16_t checkInstallationPaths(char *base_path, int16_t try_to_create_flag)
 /**
  * \brief     Vérifie que l'installation est conforme aux recommendations.
  * \details   Les répertoires suivants doivent exister et être accessibles en lecture/écriture :
  *    Si BASEPATH != /usr
  *            BASEPATH/bin
  *            BASEPATH/etc
  *            BASEPATH/lib
  *            BASEPATH/lib/plugins
  *            BASEPATH/lib/drivers
  *            BASEPATH/var
  *            BASEPATH/var/db
  *            BASEPATH/var/log
  *            BASEPATH/var/sessions
  *            BASEPATH/var/backup
  *            BASEPATH/gui
  *   Si BASEPATH == /usr (installation traditionnelle des distributions linux)
  *            /usr/bin
  *            /etc
  *            /usr/lib
  *            /usr/lib/mea-plugins
  *            /usr/lib/mea-drivers
  *            /var
  *            /var/db
  *            /var/log
  *            /tmp
  *            /var/backup
  *            /usr/lib/mea-gui
  *   Mes recommendations :
  *      BASEPATH=/usr/local/mea-edomus ou /opt/mea-edomus ou /apps/mea-edomus
  * \param     basepath            chemin vers le répertoire de base de l'installation
  * \param     try_to_create_flag  création ou non des répertoires s'il n'existe pas (0 = pas de création, 1 = création).
  * \return    0 si l'installation est ok, -1 = erreur bloquante, -2 = au moins un répertoire n'existe pas
  */
{
//    const char *default_paths_list[]={NULL,"bin","etc","lib","lib/mea-drivers","lib/mea-plugins","var","var/db","var/log","var/sessions","var/backup", "lib/mea-gui","lib/mea-rules", NULL};
    const char *default_paths_list[]={NULL,"bin","etc","lib","lib/mea-drivers","lib/mea-plugins","var","var/db","var/log","var/sessions","lib/mea-gui","lib/mea-rules", NULL};
//    const char *usr_paths_list[]={"/etc","/usr/lib/mea-plugins","/usr/lib/mea-drivers","/var/db","/var/log","/tmp","var/backup","/usr/lib/mea-gui","/usr/lib/mea-rules", NULL};
    const char *usr_paths_list[]={"/etc","/usr/lib/mea-plugins","/usr/lib/mea-drivers","/var/db","/var/log","/tmp","/usr/lib/mea-gui","/usr/lib/mea-rules", NULL};
    
    char **paths_list;
    
    char path_to_check[1024];
    int16_t flag=0;
    int16_t is_usr_flag;
    int16_t n;
    
    if(strcmp("/", base_path)==0 || base_path[0]==0) // root et pas de base_path interdit !!!
    {
        VERBOSE(2) mea_log_printf("%s (%s) : root (\"/\") and empty string (\"\") not allowed for base path\n", ERROR_STR,__func__);
        return -1;
    }

    if(strcmp(usr_str, base_path)==0) // l'installation dans /usr n'a pas la même configuration de répertoire
    {
        paths_list=(char **)usr_paths_list;
        is_usr_flag=1;
    }
    else
    {
        paths_list=(char **)default_paths_list;
        paths_list[0]=base_path; // on ajoute basepath pas dans la liste pour éventuellement pouvoir le créer.
        is_usr_flag=0;
    }
        
    for(int16_t i=0;paths_list[i];i++) {
        if(is_usr_flag || i==0) // si i==0 c'est basepath
            n=snprintf(path_to_check,sizeof(path_to_check),"%s",paths_list[i]);
       else
            n=snprintf(path_to_check, sizeof(path_to_check), "%s/%s", base_path, paths_list[i]);
        if(n<0 || n==sizeof(path_to_check)) {
            VERBOSE(9) {
               mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
               perror("");
            }
            return -1;
        }
        int16_t func_ret=access(path_to_check, R_OK | W_OK | X_OK);
        if( func_ret == -1 ) { // pas d'acces ou pas lecture/ecriture
            if(errno == ENOENT) { // le repertoire n'existe pas
                // si option de creation on essaye
                if(try_to_create_flag && mkdir(path_to_check, S_IRWXU | // xwr pour user
                                                              S_IRGRP | S_IXGRP | // x_r pour group
                                                              S_IROTH | S_IXOTH)) // x_r pour other
                {
                    VERBOSE(2) {
                        mea_log_printf("%s (%s) : %s - ",INFO_STR,__func__,path_to_check);
                        perror("");
                    }
                    flag=-1; // pas les droits en écriture
                }
                else
                    if(flag!=-1)
                        flag=-2; // repertoire n'existe pas. 1 (erreur de droit irattapable a priorité sur 2 ...)
            }
            else {
                VERBOSE(2) {
                    mea_log_printf("%s (%s) : %s - ",INFO_STR,__func__,path_to_check);
                    perror("");
                }
                flag=-1; // pas droit en écriture
            }
        }
    }
    return flag;
}


int16_t create_php_ini(char *phpini_path)
 /**
  * \brief     Création d'un fichier fichier php.ini "standard" (minimum nécessaire au fonctionnement d'un php-cgi).
  * \details   Un seul paramètre pour l'instant : timezone.
  * \param     phpini_path  chemin (complet) du fichier php.ini à créer.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur
  */
{
   char *lines[] = {
      "[Date]",
      "date.timezone = \"Europe/Berlin\"",
      NULL
   };
   
   char phpini[1024];
   int16_t n=snprintf(phpini,sizeof(phpini),"%s/php.ini",phpini_path);
   if(n<0 || n==sizeof(phpini)) {
      VERBOSE(9) {
         mea_log_printf("%s (%s) : snprintf - ", DEBUG_STR,__func__);
         perror("");
      }
      return 1;
   }

   int16_t rc=linesToFile(phpini,lines);
   if(rc)
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : cannot write %s file - ", ERROR_STR, __func__, phpini);
         perror("");
      }
   }
   return rc;
}


int16_t create_configs_php(char *base_path, char *gui_home, char *params_db_fullname, char *php_log_fullname, char *php_sessions_fullname)
 /**
  * \brief     Création d'un fichier configs.php cohérent avec l'installation de l'interface graphique et les chemins de fichiers à utiliser.
  * \details   Actuellement positionne fichier de log de php et chemin de la base sqlite3 de parametrage.
  * \param     gui_home            chemin du répertoire contenant l'arboressance des fichiers de l'interface graphique. Le fichier sera créé dans "gui_home/lib"
  * \param     params_db_fullname  nom complet et chemin de la base de paramétrage.
  * \param     php_log_fullname    nom complet et chemin du fichier de log.
  * \return    0 si OK, -1 si KO. Voir errno en cas d'erreur.
  */
{
   FILE *fd;
   const char *file="lib/configs.php";
   char *f;
   
   f=malloc(strlen(gui_home) + strlen(file) + 2); // 2 : fin de fichier + '/'
   sprintf(f, "%s/%s", gui_home,file);
   
   fd=fopen(f,"w");
   if(fd) {
      fprintf(fd, "<?php\n");
//      fprintf(fd,"ini_set('error_reporting', E_ALL);\n");
      fprintf(fd, "ini_set('error_reporting', E_ERROR);\n");
      fprintf(fd, "ini_set('log_errors', 'On');\n");
      fprintf(fd, "ini_set('display_errors', 'Off');\n");
      fprintf(fd, "ini_set('error_log', \"%s/%s\");\n",php_log_fullname, "php.log");
      fprintf(fd, "ini_set('session.save_path', \"%s\");\n", php_sessions_fullname);

      fprintf(fd, "$TITRE_APPLICATION='Mea eDomus Admin';\n");
      fprintf(fd, "$BASEPATH='%s';\n",base_path);
      fprintf(fd, "$PARAMS_DB_PATH='sqlite:%s';\n",params_db_fullname);
      fprintf(fd, "$QUERYDB_SQL='sql/querydb.sql';\n");
      fprintf(fd, "$LANG='fr';\n");
      fclose(fd);
      free(f);
      f=NULL;
   }
   else
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : cannot write %s file - ",ERROR_STR,__func__,f);
         perror("");
      }
      free(f);
      f=NULL;
      return -1;
   }
   return 0;
}


 /**
  * \brief     construit un chemin à partir de critère et insere le résultat dans la liste des parametres. Si une valeur existe déjà elle n'est pas modifiée.
  * \details   Cette fonction est utilisée par autoInit() pour simplifier le code. Elle n'a pas vocation a être utilisée par d'autres fonctions.
  * \param     params_list  liste de tous les parametres
  * \param     index        index dans params_list de la valeur à modifier.
  * \param     path         "début" du chemin à construire.
  * \param     end_path     deuxième partie du path à construire.
  * \return    0 si OK, -1 en cas d'erreur.
  */
int _construct_path(cJSON *params_list, char *key, char *path, char *end_path)
{
   char tmp_str[1024];
   char *v = appParameters_get(key, params_list);
   if(v==NULL || strlen(v)==0) {
      int16_t n=snprintf(tmp_str,sizeof(tmp_str), "%s/%s", path, end_path);
      if(n<0 || n==sizeof(tmp_str)) {
         VERBOSE(9) {
            mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
            perror("");
         }
        return -1;
      }
      appParameters_set(key, tmp_str, params_list);
   }
   return 0;
}


 /**
  * \brief     construit une chaine et insere le résultat dans la liste des parametres. Si une valeur existe déjà elle n'est pas modifiée.
  * \details   Cette fonction est utilisée par autoInit() pour simplifier le code. Elle n'a pas vocation a être utilisée par d'autres fonctions.
  * \param     params_list  liste de tous les parametres
  * \param     index        index dans params_list de la valeur à modifier.
  * \param     value        chaine à insérer.
  * \return    0 si OK, -1 en cas d'erreur.
  */
int _construct_string(cJSON *params_list, char *key, char *value)
{
   char *v = appParameters_get(key, params_list);
   if(v==NULL || strlen(v)==0) {
      appParameters_set(key, value, params_list);
   }
   return 0;
}


 /**
  * \brief     lit un chemin depuis l'entrée standard et insere le résultat dans la liste. Si une valeur existe déjà dans la list, elle est proposé comme valeur par défaut.
  * \details   Cette fonction est utilisée par interactiveInit() pour simplifier le code. Elle n'a pas vocation a être utilisée par d'autres fonctions.
  * \param     params_list  liste de tous les parametres
  * \param     index        index dans params_list de la valeur à modifier.
  * \param     base_path    "début" du chemin à proposer.
  * \param     dir_name     deuxième partie du chemin à proposer.
  * \param     question     question à poser.
  * \return    0 si OK, -1 en cas d'erreur.
  */
int _read_path(cJSON *params_list, char *key, char *base_path, char *dir_name, char *question)
{
   char tmp_str[1024];
   char *str = NULL;

   if(appParameters_get(key, params_list)) {
      strncpy(tmp_str, appParameters_get(key, params_list),sizeof(tmp_str)-1);
      str=get_and_malloc_string(tmp_str, question);
   }
   else {
      str=get_and_malloc_path(base_path, dir_name, question);
   }

   if(str) {
      appParameters_set(key, str, params_list);
      free(str);
      str=NULL;
   }

   return 0;
}


int _read_string(cJSON *params_list, char *key, char *default_value, char *question)
 /**
  * \brief     lit une chaine sur l'entrée standard et insère le résultat dans la liste des paramètres. Si une valeur existe déjà elle proposée comme valeur par défaut.
  * \details   Cette fonction est utilisée par interactiveInit() pour simplifier le code. Elle n'a pas vocation a être utilisée par d'autres fonctions.
  * \param     params_list    liste de tous les parametres
  * \param     index          index dans params_list de la valeur à modifier.
  * \param     default_value  chaine à proposer si params_list[index]==NULL.
  * \param     question       question à poser.
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
   char tmp_str[1024];
   if(appParameters_get(key, params_list)) {
      strncpy(tmp_str, appParameters_get(key, params_list),sizeof(tmp_str)-1);
   }
   else {
      strncpy(tmp_str, default_value, sizeof(tmp_str)-1);
   }

   char *str=get_and_malloc_string(tmp_str, question);
   if(str) {
      appParameters_set(key, str, params_list);
      free(str);
      str=NULL;
   }
   
   return 0;
}


int _read_integer(cJSON *params_list, char *key, int16_t default_value, char *question)
 /**
  * \brief     lit un entier sur l'entrée standard et insere le résultat dans la liste des paramètres. Si une valeur existe déjà elle proposée comme valeur par défaut.
  * \details   Cette fonction est utilisée par interactiveInit() pour simplifier le code. Elle n'a pas vocation a être utilisée par d'autres fonctions.
  * \param     params_list    liste de tous les paramètres
  * \param     index          index dans params_list de la valeur à modifier.
  * \param     default_value  chaine à proposer si params_list[index]==NULL.
  * \param     question       question à poser.
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
   char tmp_str[80];
   
   if(appParameters_get(key, params_list)) {
      strcpy(tmp_str, appParameters_get(key, params_list));
   }
   else
   {
      int16_t n=snprintf(tmp_str,sizeof(tmp_str),"%d",default_value);
      if(n<0 || n==sizeof(tmp_str)) {
         VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
         }
         return -1;
      }
   }
   
   char *str = get_and_malloc_string(tmp_str, question);
   if(str) {
      appParameters_set(key, str, params_list);
      free(str);
      str=NULL;
   }

   return 0;
}


int16_t autoInit(cJSON *params_list)
 /**
  * \brief     réalise une configuration initiale automatique de l'application.
  * \details   contrôle de l'installation, création des répertoires (si absents), création des bases et tables, alimentation des tables
  * \param     params_list  liste des paramètres
  * \return    0 si OK, code > 0 en cas d'erreur.
  */
{
   char to_check[1024];

   char *p_str; // pointeur sur une chaine
   char *sessions_str;
   int16_t retcode=0;
   int n;
   
   char *meaPath=appParameters_get("MEAPATH", params_list);

   if(strcmp(usr_str, meaPath)==0) {
      p_str="";
      sessions_str="tmp";
   }
   else {
      p_str=meaPath;
      sessions_str="var/sessions";
   }

   //
   // Mise à jour de params_list avec les valeurs par defaut pour les entrées "vide" (NULL)
   //
   char db_version[10];
   sprintf(db_version,"%d",CURRENT_PARAMS_DB_VERSION);
   _construct_string(params_list, "PARAMSDBVERSION", db_version);
   
   _construct_string(params_list, "VENDORID",        "mea");
   _construct_string(params_list, "DEVICEID",        "edomus");
   _construct_string(params_list, "INSTANCEID",      "home");
   
   _construct_string(params_list, "VERBOSELEVEL",     "1");
   _construct_string(params_list, "GUIPORT",          "8083");
   
   _construct_path(params_list,   "PHPCGIPATH",       meaPath, "bin");
   _construct_path(params_list,   "PHPINIPATH",       p_str, "etc");
   _construct_path(params_list,   "PHPSESSIONSPATH",  p_str, sessions_str);
   _construct_path(params_list,   "GUIPATH",          meaPath, "lib/mea-gui");
   _construct_path(params_list,   "PLUGINPATH",      meaPath, "lib/mea-plugins");
   _construct_path(params_list,   "DRIVERSPATH",      meaPath, "lib/mea-drivers");
   _construct_path(params_list,   "RULESFILE",        meaPath, "lib/mea-rules/automator.rules");
   _construct_path(params_list,   "RULESFILESPATH",   meaPath, "lib/mea-rules");
   _construct_path(params_list,   "LOGPATH",          p_str, "var/log");

#ifdef __linux__
   _construct_string(params_list, "INTERFACE", "eth0");
#else
   _construct_string(params_list, "INTERFACE", "en0");
#endif

   _construct_path(params_list,   "LOGPATH", p_str, "var/log");

   //
   // Contrôles et créations de fichiers
   //
   n=snprintf(to_check, sizeof(to_check), "%s/php.ini", appParameters_get("PHPINIPATH", params_list));
   if(n<0 || n==sizeof(to_check)) {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      retcode=11; goto autoInit_exit;
   }

   if( access(to_check, R_OK) == -1 ) {
      VERBOSE(9) mea_log_printf("%s (%s) : %s/php.ini - ", WARNING_STR, __func__, appParameters_get("PHPINIPATH",params_list));
      VERBOSE(9) perror("");
      VERBOSE(1) mea_log_printf("%s : no 'php.ini' exist, create one.\n", WARNING_STR);
      if(create_php_ini(appParameters_get("PHPINIPATH", params_list))) {
         retcode=11; goto autoInit_exit;
      }
   }

   n=snprintf(to_check, sizeof(to_check), "%s/php-cgi", appParameters_get("PHPCGIPATH", params_list));
   if(n<0 || n==sizeof(to_check)) {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      retcode=11; goto autoInit_exit;
   }
   
   if( access(to_check, R_OK | X_OK) == -1 ) {
      VERBOSE(9) {
         mea_log_printf("%s (%s) : %s/cgi-bin - ", WARNING_STR, __func__, appParameters_get("PHPCGIPATH", params_list));
         perror("");
      }
      VERBOSE(1) mea_log_printf("%s : no 'cgi-bin', gui will not start.\n", WARNING_STR);
   }

autoInit_exit:
   return retcode;
}


int16_t interactiveInit(cJSON *params_list)
 /**
  * \brief     réalise une configuration initiale ou une reconfiguration interactive de l'application.
  * \details   création des répertoires, création des bases et des tables, alimentation des tables à partir de données saisies par l'utilisateur
  * \param     params_list  liste de tous les paramètres
  * \return    0 si OK, code > 0 en cas d'erreur.
  */
{
   char *p_str;
   char *sessions_str;
   char to_check[1024];
   int16_t retcode=0;
   int n;

   char *meaPath=appParameters_get("MEAPATH", params_list);

   if(strcmp(usr_str,meaPath)==0) {
      p_str="";
      sessions_str="tmp";
   }
   else {
      p_str=meaPath;
      sessions_str="var/sessions";
   }

   // Récupération des données
#ifdef __linux__
   _read_string(params_list, "INTERFACE",      "eth0",      "network interface");
#else
   _read_string(params_list, "INTERFACE",      "en0",       "network interface");
#endif
   _read_string(params_list, "VENDORID",       "mea",       "xPL Vendor ID");
   _read_string(params_list, "DEVICEID",       "edomus",    "xPL Device ID");
   _read_string(params_list, "INSTANCE",       "myhome",    "xPL Instance ID");
   
   _read_path(params_list,   "GUIPATH",        meaPath, "lib/mea-gui",     "PATH to gui directory");
   _read_path(params_list,   "PLUGINPATH",     meaPath, "lib/mea-plugins", "PATH to plugins directory");
   _read_path(params_list,   "DRIVERSPATH",    meaPath, "lib/mea-plugins", "PATH to interfaces drivers directory");
   _read_path(params_list,   "RULESFILE",      meaPath, "lib/mea-rules/automator.rules", "automator executable rules file");
   _read_path(params_list,   "RULESFILESPATH", meaPath, "lib/mea-rules",   "automator rules sources files");
   _read_path(params_list,   "PHPCGIPATH",     meaPath, "bin",             "PATH to 'php-cgi' directory");
   _read_path(params_list,   "PHPINIPATH",     p_str,   "etc",             "PATH to 'php.ini' directory");
   _read_path(params_list,   "PHPSESSIONSPATH",p_str,   sessions_str,      "PATH to php sessions directory");

   _read_path(params_list,   "LOGPATH",        p_str,   "var/log",         "PATH to logs directory");
   
   _read_integer(params_list, "VERBOSELEVEL",  1,       "Verbose level");
   _read_integer(params_list, "GUIPORT",       8083,    "web interface port");

   // contrôle des données
   n=snprintf(to_check, sizeof(to_check), "%s/php-cgi", appParameters_get("PHPCGIPATH", params_list));
   if(n<0 || n==sizeof(to_check))
   {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      retcode=11; goto interactiveInit_exit;
   }

   if( access(to_check, R_OK | W_OK | X_OK) == -1 )
   {
      VERBOSE(9) {
         mea_log_printf("%s (%s) : %s/php-cgi - ", INFO_STR, __func__, appParameters_get("PHPCGIPATH", params_list));
         perror("");
      }
      VERBOSE(1) mea_log_printf("%s (%s) : no 'php-cgi', gui will not start.\n", WARNING_STR, __func__);
   }

   n=snprintf(to_check,sizeof(to_check),"%s/php.ini",appParameters_get("PHPINI_PATH", params_list));
   if(n<0 || n==sizeof(to_check)) {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR, __func__);
            perror("");
      }
      retcode=11; goto interactiveInit_exit;
   }

   if( access(to_check, R_OK ) == -1 ) {
      VERBOSE(9) {
         mea_log_printf("%s (%s) : %s/php.ini - ",INFO_STR,__func__, appParameters_get("PHPINIPATH", params_list));
         perror("");
      }
      VERBOSE(1) mea_log_printf("%s (%s) : no 'php.ini' exist, create one.\n",WARNING_STR,__func__);
      create_php_ini(appParameters_get("PHPINIPATH", params_list));
   }

   // insertion des données dans la base
   retcode=init_db(params_list)+11;

interactiveInit_exit:
   return retcode;
}


//int16_t initMeaEdomus(int16_t mode, char **params_list, char **keys)
int16_t initMeaEdomus(int16_t mode, cJSON *params_list)
 /**
  * \brief     Initialisation (ou réinitialisation) de la base de paramétrage de l'application
  * \details   Prépare et choisit le type d'initialisation à réaliser. C'est dans cette fonction que la création des répertoires peut être réalisées
  * \param     mode         mode d'initialisation : 0 = interactif, 1 = automatique
  * \param     params_list  liste de tous les parametres
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
    int16_t installPathFlag=0;
    int16_t func_ret;
    char params_db_version[10];
    char collector_id[20];

    if(!appParameters_get("MEAPATH", NULL)) {
       appParameters_set("MEAPATH", "/usr/local/mea-edomus", params_list);
    }
    
    if(mode==1) { // automatique
        installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", params_list), 1); // en auto, on essaye de créer les répertoires manquants
        if(installPathFlag==-1) {
            return 1; // les répertoires n'existent pas et n'ont pas pu être créés, installation automatique impossible
        }
        func_ret=autoInit(params_list);
    }
    else // interactif
    {
        installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", NULL), 0);
        if(installPathFlag==-2) { // au moins 1 répertoire n'existe pas (mais tous les répertoires existants sont accessibles en lecture/écriture).
            installPathFlag=-1;
            // doit-on essayer de les creer ?
            printf("Certains répertoires préconisés n'existent pas. Voulez-vous tenter de les créer ? (O/N) : ");
            char c=fgetc(stdin); // a tester
            if(c=='Y' || c=='y' || c=='O' || c=='o') {
                // installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 1); // on rebalaye mais cette fois on essaye de créer les répertoires
                installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", NULL), 1); // on rebalaye mais cette fois on essaye de créer les répertoires
                if(!installPathFlag) // pas d'erreur tous les répertoires existent maintenant en lecture/écriture.
                    printf("Les répertoires ont été créés\n");
            }
        }
        if(installPathFlag==-1)
            printf("Certains répertoires recommandés ne sont pas accessibles en lecture/écriture ou sont absents. Vous devrez choisir d'autres chemins pour l'installation\n");
        func_ret=interactiveInit(params_list);
    }
    if(func_ret)
        return -1;
    else
        return 0;
}


int16_t updateMeaEdomus(cJSON *params_list)
 /**
  * \brief     mise à jour d'un ou plusieurs paramètres de l'application.
  * \details   les paramètres à mettre à jour se trouvent dans "params_list" (uniquement les entrées non null de la table sont prises en compte)
  * \param     params_list  liste de tous les paramètres
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
   return 0;
}
