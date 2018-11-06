%{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>

#include "cJSON.h"

/* allouees/definies dans/par lex (mea-compilr2.lex) */
extern int yylineno;
extern int file_offset;
extern int line_offset;
extern char *yytext;
extern FILE *yyin;

/* globales */
// 
char *current_file = NULL;
cJSON *rulesSet = NULL;
FILE *fdi = NULL;
FILE *fdo = NULL;
int nbInput = 0;
int nbOutput = 0;
int filenum = 0;

// options de la ligne de commande
int indent_flag = 0;
int debug_flag = 0;
int verbose_flag = 0;
int optimize_flag = 0;
int jsonerror_flag = 0;
char output_file[255] = "";
char rules_path[255] = "";
char rulesset_file[255] = "";

/* prototypes de fonctions */
int yylex (void);
int yyerror(char *s);

int nl(FILE *fd);
int space(FILE *fd, int nb);
int indent(FILE *fd, int nb);

void printError(int errortojson, int num, char *file, int line, int column, char *near, char *text);

%}

%union {
   char *str;
};


/* valeurs */
%token  <str> NOMBRE
%token  <str> IDENTIFIANT
%token  <str> CHAINE
%token  <str> INSTRUCTION
%token  <str> FONCTION
%token  <str> SPECIAL
%token  <str> BOOL

/* opérateurs */
%token  <str> OPERATOR_EQ
%token  <str> OPERATOR_NE
%token  <str> OPERATOR_GE
%token  <str> OPERATOR_LE
%token  <str> OPERATOR_GT
%token  <str> OPERATOR_LW
%token  <str> AFFECTATION

/* séparateurs */
%token  <str> CROCHET_O
%token  <str> CROCHET_F
%token  <str> ACCOLADE_O
%token  <str> ACCOLADE_F
%token  <str> PARENTHESE_O
%token  <str> PARENTHESE_F
%token  <str> VIRGULE

/* instruction du langage */
%token  <str> INSTRUCTION_IS
%token  <str> INSTRUCTION_DO
%token  <str> INSTRUCTION_WITH
%token  <str> INSTRUCTION_IF
%token  <str> INSTRUCTION_ELSEIS
%token  <str> INSTRUCTION_ONMATCH
%token  <str> INSTRUCTION_ONNOTMATCH
%token  <str> INSTRUCTION_WHEN

/* mot clés "actions" */
%token  <str> ACTION_BREAK
%token  <str> ACTION_CONTINUE
%token  <str> ACTION_MOVEFORWARD

/* conditions */
%token  <str> FALL
%token  <str> RISE
%token  <str> CHANGE

/* erreur */
%token  <str> ERROR


%type <str> Valeur Fonction Fonction_parametres;
%type <str> Condition Operateur

%start Rules
%%

Rules:
  | Rules Rule
  ;

Rule:
    InputRule  { nl(fdi); indent(fdi, 2); fputc('}', fdi); /* nl(fdi); */}
  | OutputRule { nl(fdo); indent(fdo, 2); fputc('}', fdo); /* nl(fdo); */}
  ;

InputRule:
    IsBloc OnmatchBloc
  | IsBloc IfBloc OnmatchBloc OnnotmatchBloc
  | IsBloc IfBloc ElseisBloc OnmatchBloc OnnotmatchBloc
  ;

OutputRule:
  DoBloc

OnmatchBloc:
  | INSTRUCTION_ONMATCH { fputc(',', fdi); nl(fdi); indent(fdi,3); fputs("\"onmatch\":", fdi); space(fdi, 1); } MatchActions
  ;
  
OnnotmatchBloc:
  | INSTRUCTION_ONNOTMATCH { fputc(',', fdi); nl(fdi); indent(fdi, 3); fputs("\"onnotmatch\":", fdi); space(fdi, 1); } MatchActions
  ;

MatchActions:
    ACTION_BREAK { fputs("\"break\"", fdi); }
  | ACTION_CONTINUE { fputs("\"continue\"", fdi); }
  | ACTION_MOVEFORWARD IDENTIFIANT { fprintf(fdi, "\"moveforward %s\"", $2); } 
  ;

DoBloc:
    DoIdentifiant INSTRUCTION_DO DoAction INSTRUCTION_WITH PARENTHESE_O { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs("\"parameters\":", fdo); space(fdo, 1); fputc('{', fdo); nl(fdo); } DoActionParametres { nl(fdo); indent(fdo, 3); fputc('}', fdo); } PARENTHESE_F INSTRUCTION_WHEN DoCondition { space(fdo, 1); fputc('}', fdo); }
    ; 

DoIdentifiant:
    IDENTIFIANT {
                   if(nbOutput>0)
                   {
                      fputc(',',fdo);
                      nl(fdo);
                   }
                   nbOutput++;

                   indent(fdo, 2);
                   fputc('{',fdo);
                   nl(fdo);
                   indent(fdo, 3);
                   fputs("\"name\":",fdo);
                   space(fdo, 1);
                   fprintf(fdo, "\"%s\"", $1);

                   if(debug_flag == 1)
                   {
                      fputc(',',fdo);
                      nl(fdo);
                      indent(fdo, 3);
                      fputs("\"file\":", fdo);
                      space(fdo, 1);
                      fprintf(fdo, "%d,", filenum);
                      nl(fdo);
                      indent(fdo, 3);
                      fputs("\"line\":", fdo);
                      space(fdo, 1);
                      fprintf(fdo, "%d", yylineno);
                   }
                }

DoAction:
    IDENTIFIANT { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs( "\"action\":", fdo); space(fdo, 1); fprintf(fdo, "\"%s\"", $1); }

DoCondition:
   DoConditionIdentifiant DoEdge

DoConditionIdentifiant:
    IDENTIFIANT { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs("\"condition\":",fdo); space(fdo, 1); fputc('{',fdo); space(fdo,1); fprintf(fdo,"\"%s\":", $1); space(fdo, 1); }

DoEdge:
    RISE   { fprintf(fdo, "1"); }
  | FALL   { fprintf(fdo, "2"); }
  | CHANGE { fprintf(fdo, "3"); }

DoActionParametres:
  | DoActionParametre
  | DoActionParametres VIRGULE { fputc(',', fdo); nl(fdo); } DoActionParametre
  ;

DoActionParametre:
    IDENTIFIANT AFFECTATION Valeur { indent(fdo, 4); fprintf(fdo, "\"%s\": \"%s\"", $1, $3); free($3); }
  ;

IsBloc:
    IDENTIFIANT INSTRUCTION_IS Valeur  {
                                          if(nbInput>0)
                                          {
                                             fputc(',', fdi);
                                             nl(fdi);
                                          }
                                          nbInput++;

                                          indent(fdi, 2);
                                          fputc('{', fdi);
                                          nl(fdi);
                                          indent(fdi, 3); 
                                          fputs("\"name\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", $1);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"value\":",fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", $3);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"num\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi, "%d", nbInput);

                                          if(debug_flag == 1)
                                          {
                                             fputc(',',fdi); 
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"file\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d,", filenum);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"line\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d", yylineno);
                                          }

                                          free($3);
                                       }
  | IDENTIFIANT INSTRUCTION_IS SPECIAL {
                                          nbInput++;
                                          if(nbInput>0)
                                          {
                                             fputc(',', fdi);
                                             nl(fdi);
                                          }
                                          nbInput++;

                                          indent(fdi, 2);
                                          fputc('{', fdi);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"name\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", $1);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"value\":",fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", $3);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"num\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi, "%d", nbInput);

                                          if(debug_flag == 1)
                                          {
                                             fputc(',',fdi);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"file\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d,", filenum);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"line\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d", yylineno);
                                          }
 
                                          free($3);
                                       }
  ;

IfBloc:
    INSTRUCTION_IF PARENTHESE_O { fputc(',',fdi); nl(fdi); indent(fdi, 3); fputs("\"conditions\":[", fdi); nl(fdi); } Conditions PARENTHESE_F { nl(fdi); indent(fdi, 3); fputc(']', fdi); }
  ;

ElseisBloc:
    INSTRUCTION_ELSEIS Valeur { fputc(',',fdi); nl(fdi); indent(fdi, 3); fputs("\"altvalue\":", fdi); space(fdi, 1); fprintf(fdi, "\"%s\"", $2); free($2); }
  ;
 
Conditions:
    { indent(fdi, 4); fputc('{',fdi); } Condition { fputc('}',fdi); };
  | Conditions VIRGULE { fputc(',', fdi); nl(fdi); indent(fdi, 4); fputc('{', fdi); } Condition { fputc('}', fdi); }
  ;

Condition:
  Valeur Operateur Valeur {
                             fputs("\"value1\":", fdi);
                             space(fdi, 1);
                             fprintf(fdi, "\"%s\",",$1);
                             space(fdi, 1);
                             fputs("\"value2\":", fdi);
                             fprintf(fdi, "\"%s\",", $3);
                             space(fdi, 1);
                             fputs("\"op\":",fdi);
                             fprintf(fdi, "\"%s\"", $2);
                             free($3);
                          }
  ;

Operateur:
    OPERATOR_EQ { $$ = "=="; }
  | OPERATOR_NE { $$ = "!="; }
  | OPERATOR_GE { $$ = ">="; }
  | OPERATOR_LE { $$ = "<="; }
  | OPERATOR_GT { $$ = ">";  }
  | OPERATOR_LW { $$ = "<";  }
  ;

Valeur:
    NOMBRE      { char *ptr = malloc(strlen($1)+1); strcpy(ptr, $1); $$ = ptr; free($1); } 
  | IDENTIFIANT { char *ptr = malloc(strlen($1)+1); strcpy(ptr, $1); $$ = ptr; free($1); } 
  | CHAINE      { char *ptr = malloc(strlen($1)+1); strcpy(ptr, $1); $$ = ptr; free($1); }
  | BOOL        { char *ptr = malloc(strlen($1)+1); strcpy(ptr, $1); $$ = ptr; free($1); }
  | Fonction    { $$ = $1; } 
  | ACCOLADE_O IDENTIFIANT ACCOLADE_F { char *ptr = malloc(strlen($2)+3); sprintf(ptr, "{%s}", $2); $$ = ptr; free($2); }
  ;

Fonction:
  FONCTION CROCHET_O Fonction_parametres CROCHET_F {
                                                      if($3 == NULL)
                                                      {
                                                         char *ptr = (char *)malloc(strlen($1)+3);
                                                         strcpy(ptr,$1);
                                                         strcat(ptr,"[]");
                                                         $$ = ptr;
                                                      }
                                                      else
                                                      {
                                                         char *ptr = (char *)malloc(strlen($1)+strlen($3)+3);
                                                         strcpy(ptr,$1);
                                                         strcat(ptr,"[");
                                                         strcat(ptr,$3);
                                                         strcat(ptr,"]");
                                                         free($3);
                                                         $$ = ptr;
                                                      }
                                                   }
  ;

Fonction_parametres:
  { $$ = NULL; };
  | Valeur {
              char *ptr = (char *)malloc(strlen($1)+1);
              strcpy(ptr,$1);
              free($1);
              $$ = ptr;
           }
  | Fonction_parametres VIRGULE Valeur {
                                          char *ptr = (char *)realloc($1, strlen($1)+strlen($3)+2);
                                          strcat(ptr,",");
                                          strcat(ptr,$3);
                                          free($3);
                                          $$ = ptr;
                                       }
  ;
%%


int yyerror(char *s)
{
   printError(jsonerror_flag, 1, current_file, yylineno, line_offset, yytext, s);
}

/*
char ifname[255];
char ofname[255];
char line[65536];
int ret = -1;
*/
pid_t pid=-1;

/*
int indent_flag = 0;
int debug_flag = 0;
int verbose_flag = 0;
int optimize_flag = 0;
char output_file[255] = "";
char rules_path[255] = "";
char rulesset_file[255] = "";
*/
 
int indent(FILE *fd, int nb)
{
   if(indent_flag == 1)
      for(int i=0; i<nb; i++)
         fputs("   ", fd);
}


int space(FILE *fd, int nb)
{
   if(indent_flag == 1)
      for(int i=0; i<nb; i++)
         fputc(' ', fd);
}


int nl(FILE *fd)
{
   if(indent_flag == 1)
      fprintf(fd, "\n");
}


void usage(const char *name)
{
   fprintf(stderr,
      "\n"
      "Usage: %s [options] rules_files\n"
      "\n"
      "Options:\n"
      "  -h, --help            show this help message and exit\n"
      "  -o FILE, --output=FILE\n"
      "                        write result to FILE\n"
      "  -i, --indent          JSON indented output\n"
      "  -v, --verbose         verbose\n"
      "  -d, --debug           debug\n"
      "  -O, --optimize        optimize result code\n"
      "  -s RULESSETFILE, --set=RULESSETFILE\n"
      "                        from rules set\n"
      "-p RULESPATH, --rulespath=RULESPATH\n"
      "                        default rules files path\n"
      "-j, --errorjson         error in json format\n",
      name
   );
   exit(1);
}


void clean_exit(int num)
{
   exit(num);
}


void printError(int errortojson, int num, char *file, int line, int column, char *near, char *text)
{
   char *iserror = "true";
   if(num == 0)
      iserror = "false";

   if(file==NULL)
      file = "<unknown>";

   if(errortojson > 0)
   {
      printf("{ \"iserror\": %s, \"errno\": %d, \"file\": \"%s\", \"line\": %d, \"column\": %d, \"message\": \"%s\" }", iserror, num, file, line, column, text);
   }
   else
   {
     if(num!=0)
        fprintf(stderr, "Error: %s, file \"%s\" line %d, column %d, near \"%s\".\n", text, file, line, column, near);
   }
}


int main(int argc, const char * argv[])
{
   pid = getpid();
   filenum = 0;
   FILE *out = NULL;
   int out_open_flag = 0;
   char errmsg[255];
   char ifname[255];
   char ofname[255];
   char line[65536];

   int ret = -1;

   static struct option long_options[] = {
      {"help",              no_argument,       0,  'h'                  },
      {"indent",            no_argument,       0,  'i'                  },
      {"debug",             no_argument,       0,  'd'                  },
      {"verbose",           no_argument,       0,  'v'                  },
      {"optimize",          no_argument,       0,  'O'                  },
      {"errorjson",         no_argument,       0,  'j'                  },
      {"output",            required_argument, 0,  'o'                  }, // 'o'
      {"set",               required_argument, 0,  's'                  }, // 's'
      {"rulespath",         required_argument, 0,  'p'                  }, // 'p'
      {0,                   0,                 0,  0                    }
   };

   int option_index = 0;
   int c;
   while ((c = getopt_long(argc, (char * const *)argv, "hidvOjo:s:p:", long_options, &option_index)) != -1)
   {
      switch (c)
      {
         case 'h':
            usage(argv[0]);
            break;
         case 'i':
            indent_flag = 1;
            break;
         case 'd':
            debug_flag = 1;
            break;
         case 'v':
            verbose_flag = 1;
            break;
         case 'O':
            optimize_flag = 1; 
            break;
         case 'j':
            jsonerror_flag = 1; 
            break;
         case 'o':
            strcpy(output_file, optarg);
            break;
         case 'p':
            strcpy(rules_path, optarg);
            break;
         case 's':
            strcpy(rulesset_file, optarg);
            break;
         default:
            usage(argv[0]);
            break;
      }
  } 


  // récupération des noms de fichier de la ligne de commandes
  char **files = NULL;
  int nb_files = argc - optind;

  int nextSlot = 0;
  if (nb_files)
  {
     files = malloc(sizeof(char *) * (nb_files+1)); 
     while (optind < argc)
     {
        char *s = NULL;
        if(rules_path[0])
        {
           s = malloc(strlen(argv[optind])+strlen(rules_path)+strlen("/")+1);
           sprintf(s,"%s/%s", rules_path, (char *)argv[optind]);
        }
        else
        {
           s = malloc(strlen(argv[optind])+1);
           strcpy(s, argv[optind]);
        }
        files[nextSlot++]=s;
        optind++;
     }
     files[nextSlot]=NULL;
  }


  // récupération des noms de fichier d'un rulesset
  if(rulesset_file[0])
  {
     #define RS_BLOCKSIZE 200 

     FILE *rsfd = NULL;
     int rs_size = 0;
     char *rs = NULL;
     char *_rs = NULL;
     int  rs_count = 0;
     char rs_buff[21];


     rsfd = fopen(rulesset_file, "r");
     while(!feof(rsfd))
     {
        int nb=fread(rs_buff, 1, sizeof(rs_buff)-1, rsfd);
        rs_buff[nb]=0;
        if((rs_count + nb) >= rs_size)
        {
           rs_size=rs_size+RS_BLOCKSIZE;
           _rs = realloc(rs, rs_size);
           if(!_rs)
           {
              snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't load json file", strerror(errno));
              printError(jsonerror_flag, 254, rulesset_file, 0, 0, "N/A", errmsg);
              clean_exit(1);
           }
           rs = _rs;
        }
        memcpy(&(rs[rs_count]), rs_buff, nb);
        rs_count+=nb;
     } 
     fclose(rsfd);

     rulesSet = cJSON_Parse(rs);

     free(rs);

     if(rulesSet)
     {
        cJSON *e=rulesSet->child;
        while(e)
        {
           char **_files = realloc(files, sizeof(char *) * (nextSlot + 1) + sizeof(char *) );
           if(!_files)
           {
              snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create rules files list", strerror(errno));
              printError(jsonerror_flag, 253, rulesset_file, 0, 0, "N/A", errmsg);
              clean_exit(1);
           } 
           files=_files;
           char *s = NULL;
           if(rules_path[0])
           {
              s = malloc(strlen(e->valuestring)+strlen(rules_path)+strlen(".srules")+strlen("/")+1);
              sprintf(s, "%s/%s.srules", rules_path, e->valuestring);
           }
           else
           {
              s = malloc(strlen(e->valuestring)+strlen(".srules")+1);
              sprintf(s, "%s.srules", e->valuestring);
           }
           files[nextSlot]=s;
           nextSlot++;
           e=e->next;
        }

        if(files)
           files[nextSlot]=NULL;

        cJSON_Delete(rulesSet);
     }

  }

  if(!files) // pas de fichier trouvé
  {
     printError(jsonerror_flag, 255, "N/A", 0, 0, "N/A", "no input file");
     if(jsonerror_flag == 0)
        usage(argv[0]);
  }


  // récupération du nom de fichier de sortie
  if(output_file[0]==0)
     out = stdout;
  else
  {
     char _output_file[255];
     if(rules_path[0])
        snprintf(_output_file, sizeof(_output_file)-1, "%s/%s", rules_path, output_file);
     else
        strncpy(_output_file, output_file, sizeof(_output_file)-1);
     out = fopen(_output_file, "w");
     if(out == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create file", strerror(errno));
        printError(jsonerror_flag, 254, _output_file, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     out_open_flag = 1;
  }


  // traitement des fichiers
  for(filenum=0;files[filenum];filenum++)
  {
     if(verbose_flag == 1) fprintf(stderr,"Processing file : %s\n", files[filenum]);
     current_file = NULL;
     yyin=fopen(files[filenum],"r");
     if(yyin==NULL)
     {
        sprintf(errmsg, "%s (%s)", "can't open file", strerror(errno));
        printError(jsonerror_flag, 2, files[filenum], 0, 0, "N/A", errmsg);
        clean_exit(1);
     } 
     current_file = files[filenum];

     sprintf(ifname, "/tmp/input.%d.%d.tmp", filenum, pid);
     sprintf(ofname, "/tmp/output.%d.%d.tmp", filenum, pid);

     fdi = fopen(ifname,"w+");
     if(fdi == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create tmp file", strerror(errno));
        printError(jsonerror_flag, 3, ifname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }

     fdo = fopen(ofname,"w+");
     if(fdo == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create tmp file", strerror(errno));
        printError(jsonerror_flag, 4, ofname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }

     file_offset=0;
     line_offset=0;
     yylineno=1;
     ret = yyparse();

     if(ret != 0)
     {
        clean_exit(1);
     }

     fclose(fdo);
     fdo = NULL;
     fclose(fdi);
     fdi = NULL;
  }


  // constitution du fichier final
  fputc('{', out);
  nl(out);

  // entrees
  indent(out,1); fprintf(out, "\"inputs\":["); nl(out);
  for(filenum=0;files[filenum];filenum++)
  {
     sprintf(ifname, "/tmp/input.%d.%d.tmp", filenum, pid);

     fdi = fopen(ifname, "r");
     if(fdi == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't open tmp file", strerror(errno));
        printError(jsonerror_flag, 5, ifname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     while(!feof(fdi))
     {
        line[0]=0;
        fgets(line, sizeof(line)-1, fdi);
        fputs(line, out);
     }
     fclose(fdi);

     unlink(ifname); // suppression du fichier temporaire
  }
  nl(out);
  indent(out,1);
  fputc(']', out);


  fputc(',', out);
  nl(out);

  // sorties
  indent(out,1); fprintf(out, "\"outputs\":["); nl(out);
  for(filenum=0;files[filenum];filenum++)
  {
     sprintf(ofname, "/tmp/output.%d.%d.tmp", filenum, pid);
     fdo = fopen(ofname, "r");
     if(fdo == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't open tmp file", strerror(errno));
        printError(jsonerror_flag, 5, ofname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     while(!feof(fdo))
     {
        line[0]=0;
        fgets(line, sizeof(line)-1, fdo);
        fputs(line, out);
     }
     unlink(ofname); // suppression du fichier temporaire
  }
  nl(out);
  indent(out,1);
  fputc(']',out);

  // noms des fichiers
  if(files && debug_flag == 1)
  {
     fputc(',', out);
     nl(out);
     indent(out, 1);
     fputs("\"files\":[", out);
     for(int i=0;files[i];i++)
     {
        if(i!=0)
           fputc(',', out);
        nl(out);
        indent(out, 2);
        fprintf(out, "\"%s\"", files[i]);
     
        free(files[i]);
     }
     free(files);
  }
  nl(out);
  indent(out,1);
  fputc(']',out);

  nl(out);
  fputc('}', out);
  nl(out);

  if(out_open_flag == 1)
     fclose(out);

  printError(jsonerror_flag, 0, "N/A", 0, 0, "N/A", "no error");

  exit(0);
}
