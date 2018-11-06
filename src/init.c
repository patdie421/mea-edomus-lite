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
   if(fd)
   {
      int error_flag=0;
      for(int16_t i=0;lines[i];i++)
      {
         if(fprintf(fd,"%s\n",lines[i])<0)
         {
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

   do
   {
      printf("%s [%s] : ", question_str, default_value);
      if(fgets(read_str,sizeof(read_str), stdin))
      {
         read_str[strlen(read_str)-1]=0; // retire le \n à la fin de la ligne
         read_str_trimed=mea_strtrim(read_str); // retire les blanc en début et fin de chaine
         if(!strlen(read_str_trimed)) // appuie sur Enter sans saisir de caractères
         {
            strcpy(read_str, default_value); // on prend la valeur qu'on a prosée
            read_str_trimed=read_str;
         }
      }
      else
      {
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

   if(base_path)
   {
      int16_t n=snprintf(proposed_path_str,sizeof(proposed_path_str), "%s/%s", base_path, dir_name);
      if(n<0 || n==sizeof(proposed_path_str))
      {
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
   if(n<0 || n==sizeof(default_value_str))
   {
      VERBOSE(9) {
         mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
         perror("");
      }
      return NULL;
   }

   while(1)
   {
      value_str=get_and_malloc_string(default_value_str, question_str);
      if(!value_str)
         return NULL;
      strtol(value_str,&end,10);
      if(*end!=0 || errno==ERANGE)
      {
         free(value_str);
         value_str=NULL;
      }
      else
         break;
   }
   
   return value_str;
}


int16_t checkParamsDb(char *sqlite3_db_param_path, int16_t *cause)
 /**
  * \brief     Contrôle l'état de la base de parametrage.
  * \details   pour l'instant contrôle de la présence et de l'accessibilité en lecture/écriture uniquement
  * \param     sqlite3_db_param_path  chemin complet vers le fichier de la base
  * \param     cause                  en cas d'erreur contient le code de l'erreur
  * \return    0 si OK, -1 en cas d'erreur.
  */
{
   sqlite3 *sqlite3_param_db;

   if( access( sqlite3_db_param_path, F_OK) == -1 ) // file exist ?
   {
      VERBOSE(1) mea_log_printf("%s (%s) : \"%s\" n'existe pas ou n'est pas accessible.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=1;
      return -1;
   }
   
   if( access( sqlite3_db_param_path, R_OK | W_OK) == -1 )
   {
      VERBOSE(1) mea_log_printf("%s (%s) : \"%s\" doit être accessible en lecture/ecriture.\n",ERROR_STR,__func__,sqlite3_db_param_path);
      *cause=2;
      return -1;
   }

   int16_t ret = sqlite3_open_v2(sqlite3_db_param_path, &sqlite3_param_db, SQLITE_OPEN_READONLY, NULL);
   if(ret)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      *cause=3;
      return -1;
   }

   // contrôler le contenu de la base ... à faire ???
   
   sqlite3_close(sqlite3_param_db);
   
   return 0;
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
        
    for(int16_t i=0;paths_list[i];i++)
    {
        if(is_usr_flag || i==0) // si i==0 c'est basepath
            n=snprintf(path_to_check,sizeof(path_to_check),"%s",paths_list[i]);
       else
            n=snprintf(path_to_check, sizeof(path_to_check), "%s/%s", base_path, paths_list[i]);
        if(n<0 || n==sizeof(path_to_check))
        {
            VERBOSE(9) {
               mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
               perror("");
            }
            return -1;
        }
        int16_t func_ret=access(path_to_check, R_OK | W_OK | X_OK);
        if( func_ret == -1 ) // pas d'acces ou pas lecture/ecriture
        {
            if(errno == ENOENT) // le repertoire n'existe pas
            {
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
            else
            {
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
   if(n<0 || n==sizeof(phpini))
   {
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


// int16_t create_configs_php(char *base_path, char *gui_home, char *params_db_fullname, char *php_log_fullname, char *php_sessions_fullname, int iosocket_port)
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
   if(fd)
   {
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
//      fprintf(fd, "$IOSOCKET_PORT=%d;\n",iosocket_port);
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


// int16_t create_queries_db(char *queries_db_path)
 /**
  * \brief     Création de la base et de la table queries.
  * \param     queries_db_path  chemin vers la base
  * \return    0 si OK, -1 si KO.
  */
/*
{
   sqlite3 *sqlite3_queries_db;
   
   char *sql_createTables[] = {
      "CREATE TABLE queries ( id INTEGER PRIMARY KEY, request TEXT )",
      NULL
   };
   
   if(sqlite3_dropDatabase(queries_db_path))
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : sqlite3_dropDatabase - ", ERROR_STR,__func__);
         perror("");
      }
   }
   
   int16_t nerr = sqlite3_open(queries_db_path, &sqlite3_queries_db);
   if(nerr)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_queries_db));
   }

   nerr=sqlite3_doSqlQueries(sqlite3_queries_db, sql_createTables);

   if(sqlite3_queries_db)
        sqlite3_close(sqlite3_queries_db);

   return nerr;
}
*/


int16_t createMeaTables(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Création des tables de l'application dans la base sqlite3 de paramétrage.
  * \details   Les tables application_parameters, interfaces, locations, sensors_actuators, types, sessions et users sont créées à vide.
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_createTables[] = {
//      "CREATE TABLE application_parameters(id INTEGER PRIMARY KEY,key TEXT,value TEXT,complement TEXT)",
      "CREATE TABLE interfaces(id INTEGER PRIMARY KEY,id_interface INTEGER,id_type INTEGER,name TEXT,description TEXT,dev TEXT,parameters TEXT,state INTEGER)",
      "CREATE TABLE locations(id INTEGER PRIMARY KEY,id_location INTEGER,name TEXT,description TEXT, deleted_flag INTEGER)",
      "CREATE TABLE sensors_actuators(id INTEGER PRIMARY KEY,id_sensor_actuator INTEGER,id_type INTEGER,id_interface INTERGER,name TEXT,description TEXT,id_location INTEGER,parameters TEXT,state INTEGER, todbflag INTEGER, deleted_flag INTEGER)",
      "CREATE TABLE types(id INTEGER PRIMARY KEY,id_type INTEGER,name TEXT,description TEXT,parameters TEXT,flag INTEGER,typeoftype INTEGER)",
      "CREATE TABLE sessions (id INTEGER PRIMARY KEY, userid TEXT, sessionid INTEGER, lastaccess DATETIME)",
      "CREATE TABLE users (id INTEGER PRIMARY KEY, id_user INTEGER, name TEXT, password TEXT, description TEXT, profil INTEGER, language TEXT, flag INTEGER)",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_createTables);
}


int16_t populateMeaUsers(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les utilisateurs par défaut dans la table des utilisateurs.
  * \details   1 seul utilisateur est créé (ou recréé après suppression) pour le moment : admin.
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_usersTable[] = {
      "DELETE FROM 'users' WHERE name='admin'",
      "INSERT INTO 'users' (name, password, description, profil, flag, language) VALUES ('admin','admin','Default administrator','1','1', 'default')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int16_t populateMeaLocations(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les lieux par défaut dans la table des lieux.
  * \details   1 seul lieu est créé (ou recréé après suppression) pour le moment : unknown (ie pas de localisation précisée).
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   char *sql_usersTable[] = {
      "DELETE FROM 'locations' WHERE name COLLATE nocase='unknown'",
      "INSERT INTO 'locations' (id_location, name, description) VALUES ('1','unknown','')",
      NULL
   };

   return sqlite3_doSqlQueries(sqlite3_param_db, sql_usersTable);
}


int addNewTypes(sqlite3 *sqlite3_param_db, struct types_value_s *types_values)
{
   char sql[1024];
   int16_t rc = 0;
   char *err = NULL;
   
   for(int16_t i=0;types_values[i].name;i++)
   {
      int16_t ret;
      
      int16_t n=snprintf(sql,sizeof(sql),"DELETE FROM 'types' WHERE name='%s'",types_values[i].name);
      if(n<0 || n==sizeof(sql))
      {
         VERBOSE(9) {
            mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
            perror("");
         }
         rc=1;
         break;
      }

      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) mea_log_printf ("%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_free(err);
         rc=1;
         break;
      }
      
      n=snprintf(sql, sizeof(sql), "INSERT INTO 'types' (id_type,name,description,parameters,flag,typeoftype) VALUES (%d,'%s','%s','%s','%s','%s')",types_values[i].id_type,types_values[i].name,types_values[i].description,types_values[i].parameters,types_values[i].flag,types_values[i].typeoftype);
      if(n<0 || n==sizeof(sql))
      {
         VERBOSE(9) {
            mea_log_printf ("%s (%s) : snprintf - ", DEBUG_STR,__func__);
            perror("");
         }
         rc=1;
         break;
      }

      ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
      if( ret != SQLITE_OK )
      {
         VERBOSE(9) mea_log_printf ("%s (%s) : sqlite3_exec - %s (%s)\n", DEBUG_STR,__func__,sqlite3_errmsg (sqlite3_param_db),sql);
         sqlite3_free(err);
         rc=1;
         break;
      }
   }
   return rc;
}


int populateMeaTypes(sqlite3 *sqlite3_param_db)
 /**
  * \brief     Ajoute les types par défaut dans la table des types.
  * \details   Il s'agit des types utilisés directement par l'application (interface_001 ou interface_002 pour le moment).
  * \param     sqlite3_param_db  descripteur de la base
  * \return    0 si OK, -1 si KO.
  */
{
   struct types_value_s types_values[] = {
      {INTERFACE_TYPE_001,"INTYP01","Interface de type 01","","1","10"},
      {INTERFACE_TYPE_002,"INTYP02","Interface de type 02","","1","10"},
      {INTERFACE_TYPE_003,"INTYP03","Interface de type 03","","1","10"},
      {INTERFACE_TYPE_004,"INTYP04","Interface de type 04","","1","10"},
      {INTERFACE_TYPE_005,"INTYP05","Interface de type 05","","1","10"},
      {INTERFACE_TYPE_006,"INTYP06","Interface de type 06","","1","10"},
//      {INTERFACE_TYPE_010,"INTYP10","Interface de type 10","","1","10"},
      {201,"XBEECA","Capteurs et actionneurs a interface XBee","","1","10"},
      {1000,"PWRCTR","Capteur de compteur ERDF","","1","0"},
      {1001,"TYP1IN","Entree interface type 01","","1","0"},
      {1002,"TYP1OUT","Sortie interface type 01","","1","1"},
      {2000,"XDHT22H","Humidité de DTH22","","2","0"},
      {2001,"XDHT22T","Température de DTH22","","2","0"},
      {2002,"XDHT22P","Pile de DTH22","","2","0"},
      {500, "INPUT", "Entrée", "","1","0"},
      {501, "OUTPUT", "Sortie", "", "1","1"},
      {0,NULL,NULL,NULL,NULL,NULL}
   };
   
   return addNewTypes(sqlite3_param_db, types_values);
}


//int16_t init_db(char **params_list, char **keys)
int16_t init_db(cJSON *params_list)
 /**
  * \brief     initialisation de la table "application_parameters".
  * \details   Les différents paramètres nécessaires au lancement de l'application sont insérés dans la base. La base en complétement initialisée. Si elle existe deja elle est supprimer et recréé.
  * \param     params_list  liste de parametres à mettre dans la base
  * \return    0 si OK, un code de 1 à 10 en cas d'erreur.
  */
{
   sqlite3 *sqlite3_param_db=NULL;
   int16_t retcode=0;
   int16_t func_ret;
   char sql_query[1024];
   char *errmsg = NULL;    

//DBSERVER   create_queries_db(params_list[SQLITE3_DB_BUFF_PATH]);

   // suppression de la base si elle existe déjà
//   func_ret = sqlite3_dropDatabase(params_list[SQLITE3_DB_PARAM_PATH]);
   func_ret = sqlite3_dropDatabase(appParameters_get("SQLITE3DBPARAMPATH", params_list));
   if(func_ret!=0 && errno!=ENOENT)
   {
      retcode=1;
      goto exit_init_db;
   }
    
   // création de la base
//   int16_t nerr = sqlite3_open(params_list[SQLITE3_DB_PARAM_PATH], &sqlite3_param_db);
   int16_t nerr = sqlite3_open(appParameters_get("SQLITE3DBPARAMPATH", params_list), &sqlite3_param_db);
   if(nerr)
   {
      VERBOSE(5) mea_log_printf ("%s (%s) : sqlite3_open - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
      retcode=2;
      goto exit_init_db;
   }
    
   // création des tables
   if(createMeaTables(sqlite3_param_db))
   {
      retcode=3;
      goto exit_init_db;
   }
    
   // Chargement des types standards
   if(populateMeaTypes(sqlite3_param_db))
   {
      retcode=4;
      goto exit_init_db;
   }
   
   if(populateMeaLocations(sqlite3_param_db))
   {
      retcode=5;
      goto exit_init_db;
   }

   if(populateMeaUsers(sqlite3_param_db))
   {
      retcode=6;
      goto exit_init_db;
   }

   char str[1024]="";
   snprintf(str, sizeof(str)-1, "%s/etc/mea-edomus.json.test", appParameters_get("MEAPATH", params_list));
   writeJson(str, params_list);

exit_init_db:
    if(sqlite3_param_db)
        sqlite3_close(sqlite3_param_db);
    return retcode;
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
/*
   if(!params_list[index])
   {
      params_list[index]=malloc(strlen(value)+1);
      strcpy(params_list[index],value);
   }
*/
//   if(!appParameters_get(key, params_list)) {
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
   
//   if(params_list[index])
   if(appParameters_get(key, params_list))
   {
      strcpy(tmp_str, appParameters_get(key, params_list));
//      free(params_list[index]);
//      params_list[index]=NULL;
   }
   else
   {
      int16_t n=snprintf(tmp_str,sizeof(tmp_str),"%d",default_value);
      if(n<0 || n==sizeof(tmp_str))
      {
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

   if(strcmp(usr_str, meaPath)==0)
   {
      p_str="";
      sessions_str="tmp";
   }
   else
   {
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
   if(n<0 || n==sizeof(to_check))
   {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      retcode=11; goto autoInit_exit;
   }

   if( access(to_check, R_OK) == -1 )
   {
      VERBOSE(9) mea_log_printf("%s (%s) : %s/php.ini - ", WARNING_STR, __func__, appParameters_get("PHPINIPATH",params_list));
      VERBOSE(9) perror("");
      VERBOSE(1) mea_log_printf("%s : no 'php.ini' exist, create one.\n", WARNING_STR);
      if(create_php_ini(appParameters_get("PHPINIPATH", params_list))) {
         retcode=11; goto autoInit_exit;
      }
   }

   n=snprintf(to_check, sizeof(to_check), "%s/php-cgi", appParameters_get("PHPCGIPATH", params_list));
   if(n<0 || n==sizeof(to_check))
   {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR,__func__);
            perror("");
      }
      retcode=11; goto autoInit_exit;
   }
   
   if( access(to_check, R_OK | X_OK) == -1 )
   {
      VERBOSE(9) {
         mea_log_printf("%s (%s) : %s/cgi-bin - ", WARNING_STR, __func__, appParameters_get("PHPCGIPATH", params_list));
         perror("");
      }
      VERBOSE(1) mea_log_printf("%s : no 'cgi-bin', gui will not start.\n", WARNING_STR);
   }

   //
   // mise à jour de la base (table application_parameters
   //
   retcode=init_db(params_list)+11;

autoInit_exit:
   return retcode;
}


//int16_t interactiveInit(char **params_list, char **keys)
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

   if(strcmp(usr_str,meaPath)==0)
   {
      p_str="";
      sessions_str="tmp";
   }
   else
   {
      p_str=meaPath;
      sessions_str="var/sessions";
   }

//   char db_version[10];
//   sprintf(db_version,"%d",CURRENT_PARAMS_DB_VERSION);

//   _construct_string(params_list, PARAMSDBVERSION, db_version);

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
//   n=snprintf(to_check,sizeof(to_check), "%s/php-cgi", params_list[PHPCGI_PATH]);
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
//         mea_log_printf("%s (%s) : %s/php-cgi - ", INFO_STR, __func__, params_list[PHPCGI_PATH]);
         mea_log_printf("%s (%s) : %s/php-cgi - ", INFO_STR, __func__, appParameters_get("PHPCGIPATH", params_list));
         perror("");
      }
      VERBOSE(1) mea_log_printf("%s (%s) : no 'php-cgi', gui will not start.\n", WARNING_STR, __func__);
   }

//   n=snprintf(to_check,sizeof(to_check),"%s/php.ini",params_list[PHPINI_PATH]);
   n=snprintf(to_check,sizeof(to_check),"%s/php.ini",appParameters_get("PHPINI_PATH", params_list));
   if(n<0 || n==sizeof(to_check))
   {
      VERBOSE(2) {
            mea_log_printf ("%s (%s) : snprintf - ", ERROR_STR, __func__);
            perror("");
      }
      retcode=11; goto interactiveInit_exit;
   }

   if( access(to_check, R_OK ) == -1 )
   {
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
/* 
    if(!params_list[MEA_PATH]) {
        mea_string_free_alloc_and_copy(&params_list[MEA_PATH], "/usr/local/mea-edomus");
    }
*/
    if(!appParameters_get("MEAPATH", NULL)) {
       appParameters_set("MEAPATH", "/usr/local/mea-edomus", params_list);
    }
//    snprintf(params_db_version,sizeof(params_db_version)-1,"%d",CURRENT_PARAMS_DB_VERSION);
//    mea_string_free_alloc_and_copy(&params_list[PARAMSDBVERSION], params_db_version);
//    snprintf(collector_id,sizeof(collector_id)-1,"%lu",(unsigned long)time(NULL));
//    mea_string_free_alloc_and_copy(&params_list[COLLECTOR_ID], collector_id);
    
    if(mode==1) // automatique
    {
        installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", params_list), 1); // en auto, on essaye de créer les répertoires manquants
        if(installPathFlag==-1)
        {
            return 1; // les répertoires n'existent pas et n'ont pas pu être créés, installation automatique impossible
        }
//        func_ret=autoInit(params_list, keys);
        func_ret=autoInit(params_list);
    }
    else // interactif
    {
//        installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 0);
        installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", NULL), 0);
        if(installPathFlag==-2) // au moins 1 répertoire n'existe pas (mais tous les répertoires existants sont accessibles en lecture/écriture).
        {
            installPathFlag=-1;
            // doit-on essayer de les creer ?
            printf("Certains répertoires préconisés n'existent pas. Voulez-vous tenter de les créer ? (O/N) : ");
            char c=fgetc(stdin); // a tester
            if(c=='Y' || c=='y' || c=='O' || c=='o')
            {
                // installPathFlag=checkInstallationPaths(params_list[MEA_PATH], 1); // on rebalaye mais cette fois on essaye de créer les répertoires
                installPathFlag=checkInstallationPaths(appParameters_get("MEAPATH", NULL), 1); // on rebalaye mais cette fois on essaye de créer les répertoires
                if(!installPathFlag) // pas d'erreur tous les répertoires existent maintenant en lecture/écriture.
                    printf("Les répertoires ont été créés\n");
            }
        }
        if(installPathFlag==-1)
            printf("Certains répertoires recommandés ne sont pas accessibles en lecture/écriture ou sont absents. Vous devrez choisir d'autres chemins pour l'installation\n");
//        func_ret=interactiveInit(params_list, keys);
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


static int16_t _setVersion(sqlite3 *sqlite3_param_db, int version)
{
 char *err = NULL;
 int ret;
 char sql[256];
 int16_t n;
 
   n=snprintf(sql, sizeof(sql), "UPDATE 'application_parameters' set value = '%d' WHERE key = 'PARAMSDBVERSION'", version);

   ret = sqlite3_exec(sqlite3_param_db, sql, NULL, NULL, &err);
   if( ret != SQLITE_OK )
   {
      VERBOSE(9) mea_log_printf ("%s (%s) : sqlite3_exec - %s\n", DEBUG_STR,__func__,sqlite3_errmsg(sqlite3_param_db));
      sqlite3_free(err);
      return -1;
   }
   
   return 0;
}


typedef int16_t (*upgrade_params_db_from_x_to_y_f)(sqlite3 *, struct upgrade_params_s *);

upgrade_params_db_from_x_to_y_f upgrade_params_db_from_x_to_y[]= {
//   upgrade_params_db_from_0_to_1,
//   upgrade_params_db_from_1_to_2,
   NULL };


int16_t upgrade_params_db(sqlite3 *sqlite3_param_db, uint16_t fromVersion, uint16_t toVersion, struct upgrade_params_s *upgrade_params)
{
   int i=0;
   for(i=fromVersion; (upgrade_params_db_from_x_to_y[i] && i<toVersion); i++)
   {
     if(upgrade_params_db_from_x_to_y[i](sqlite3_param_db, upgrade_params)<0)
     {
       VERBOSE(1) mea_log_printf ("%s (%s) : can't upgrade database. Sorry, you have to upgrade manually.\n", ERROR_STR,__func__);
       return -1;
       break;
     }
   }
   
   return 0;
}
