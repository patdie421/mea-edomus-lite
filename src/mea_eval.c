//
//  mea_eval.c
//
//  Created by Patrice DIETSCH on 10/12/15.
//
//
#define DEBUGFLAGON 1

//#define EVAL_MODULE_TEST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "mea_string_utils.h"
#include "automator.h"
#include "mea_eval.h"
#include "uthash.h"

static void *mea_eval_userdata = NULL;
static getVarVal_f _getVarVal = NULL;
static getVarId_f _getVarId = NULL;


static inline int  mea_eval_operatorPriorityCmpN(int op1, int op2);
static inline int  mea_eval_getOperatorN(char *str, char **newptr);
static inline void mea_eval_getSpace(char *p, char **newptr);
static inline int16_t  mea_eval_getFunctionId(char *str, int16_t l);


enum mea_eval_function_id_e { INT_F=0 };


struct mea_eval_function_def_s {
   char *name;
   enum mea_eval_function_id_e num;
   uint16_t l;
};


struct mea_eval_function_def_s functionsList[]={
   { "int", INT_F, 3 },
   { NULL, 0, 0 }
};


enum mea_eval_token_type_id_e { NUMERIC_T, VARIABLE_T, FUNCTION_T };

union mea_eval_token_data_u {
   double floatval;
   int16_t var_id;
   int16_t fn_id;
};


static void mea_eval_getSpace(char *p, char **newptr)
{
   while(*p && isspace(*p)) p++;
   *newptr=p;
}


static int mea_eval_getOperatorN(char *str, char **newptr)
{
   char *p = str;
   *newptr = p;

   switch(*p) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '%':
         *newptr=p+1;
         return(*p);
   }
   return -1;
}


static int16_t mea_eval_getFunctionId(char *str, int16_t l)
{
   int functionId = -1;
   if(l>1024)
      return -1;
   char *name = alloca(l+1);

   strncpy(name, str, l);
   name[l]=0;

   for(int i=0; functionsList[i].name; ++i) {
      if(strcmp(functionsList[i].name, name)==0) {
         functionId=functionsList[i].num+128;
         break;
      }
   }
   return functionId;
}


static int mea_eval_callFunction(double *d, int fn, double d1)
{
   switch(fn-128) {
      case INT_F: // int()
        *d=(double)((int)d1);
        break;
      default:
        return -1;
   }
   return 0;
}


/*
Un token pour une formule "numérique" est soit :
- un nombre réel : NUMERIC_T (#x)
- une variable ({nomVar}) : VARIABLE_T => le numero de la variable doit être retournée par _getVarId(nomVar, ...)
- une fonction (fn(x)) : FUNCTION_T
*/
static enum mea_eval_token_type_id_e mea_eval_getToken(char *str, char **newptr, union mea_eval_token_data_u *v)
{
   char *p = str;
   *newptr=p;
 
   switch(*p) {
      case '#': {
         char *n = NULL;
         ++p;
         double d=strtod(p, &n);
         if(p!=n) {
            v->floatval=d;
            *newptr=n;
            return NUMERIC_T;
         }
         else
            return -1;
      }
      case '{': {
         ++p;
         while(*p && *p!='}')
            ++p;
         if(*p=='}') {
            char name[VALUE_MAX_STR_SIZE];
            strncpy(name, str+1, (int)(p-str)-1);
            name[(int)(p-str-1)]=0; 

            if(_getVarId != NULL) {
               int16_t id = 0;
               if(_getVarId(name, mea_eval_userdata, &id)<0) {
                  return -1;
               }
               v->var_id=id;
            }
            p++;
            *newptr=p;
            return VARIABLE_T;
         }
         else
            return -1;
      }

      default: {
         ++p;

         if(!isalpha(*p))
            return -1;
         ++p;
         while(*p && isalpha(*p))
            ++p;
         int16_t ret=mea_eval_getFunctionId(str, (int16_t)(p-str));
         if(ret>=0) {
            v->fn_id=ret;
            *newptr=p;
            return FUNCTION_T;
         }
         else
            return -1;
      }
   }
   return -1;
}


static int mea_eval_doOperationN(double *d, double d1, int op, double d2)
{
   switch(op) {
      case '+':
         *d=d1+d2;
         break;
      case '-':
         *d=d1-d2;
         break;
      case '/':
          if(d2==0.0)
             return -1;
         *d=d1/d2;
         break;
      case '%':
          if(d2==0.0)
             return -1;
         *d=(double)((long)d1 % (long)d2);
         break;
      case '*':
         *d=d1*d2;
         break;
      default:
         return -1;
   }
   return 0;
}


static int mea_eval_getOperatorPriorityN(int op)
{
   if(op>127) // c'est un id fonction
      return 3;

   switch(op) {
      case '+':
      case '-':
         return 1;
      case '*':
      case '/':
      case '%':
         return 2;
      default:
         return -1;
   }
}



static int mea_eval_operatorPriorityCmpN(int op1, int op2)
{
   return mea_eval_getOperatorPriorityN(op1) - mea_eval_getOperatorPriorityN(op2);
}


static int pushToEvalStack(int type, void *value, struct mea_eval_stack_s **stack, int *stack_size, int *stack_index)
{
   ++(*stack_index);
   if(*stack_index >= *stack_size) {
      *stack_size = *stack_size + 10;
      struct mea_eval_stack_s *tmp;
      tmp = *stack;
      *stack = (struct mea_eval_stack_s *)realloc(*stack, *stack_size * sizeof(struct mea_eval_stack_s));
      if(*stack == NULL) {
         *stack = tmp;
         return -1;
      }
   }

   struct mea_eval_stack_s *s=*stack;

   s[*stack_index].type=type;
   switch(type) {
      case 1:
         s[*stack_index].val.value=*((double *)value);
         break;
      case 2:
         s[*stack_index].val.op=*((int *)value);
         break;
      case 3:
         s[*stack_index].val.id=*((int *)value);
         break;
      default:
         return -1;
   }

   return 0;
}


#ifndef ONFLYEVAL
static int mea_eval_calcOperationN(int op, struct mea_eval_stack_s *stack, int *stack_size, int *stack_index)
{
   double d, d1, d2;

   if(op<128) { // c'est un opérateur
      d2=stack[(*stack_index)--].val.value;
      d1=stack[(*stack_index)--].val.value;
      if(mea_eval_doOperationN(&d, d1, op, d2)<0)
         return -1;
   }
   else { // c'est une fonction ou une variable
      if(op==255) { // c'est une variable
         d1=stack[(*stack_index)--].val.value;
         if(_getVarVal != NULL && _getVarVal((int)d1, mea_eval_userdata, &d)<0)
            return -1; 
      }
      else {
         d1=stack[(*stack_index)--].val.value;
         if(mea_eval_callFunction(&d, op, d1)<0)
            return -1;
      }
   }
   pushToEvalStack(1, (void *)&d, &stack, stack_size, stack_index);
   
   return 0;
}
#endif


static int _evalCalcN(char *str, char **newptr, int16_t *lvl, struct mea_eval_stack_s *stack, int32_t *stack_size, int32_t *stack_index, int16_t *err)
{
   int operators[10];
   int operators_index=-1;

   *err = 0;
   if(!*str) {
      *err=1; // pas de ligne à évaluer
      return -1;
   }

   if(*lvl>10) {
      *err=2; // trop de niveau de parenthèse
      return -1;
   }

   char *p=str;
   char *s;

   do {
      s=p;
      mea_eval_getSpace(s, &p);
      
      s=p;
      if(*s=='(') {
         ++s;
         *lvl=*lvl+1;
         if(_evalCalcN(s, &p, lvl, stack, stack_size, stack_index, err)<0)
            return -1;
         *lvl=*lvl-1;
         
         s=p;
         mea_eval_getSpace(s, &p);
         
         if(*p==')')
            ++p;
         else {
            *newptr=p;
            *err=3; // parenthèse fermante attendu (expression)
            return -1;
         }
      }
      else {
         union mea_eval_token_data_u v;
         int ret=mea_eval_getToken(s, &p, &v);

         if(ret==NUMERIC_T) {
            pushToEvalStack(1, (void *)&(v.floatval), &stack, stack_size, stack_index);
         }
         else if(ret==VARIABLE_T)
         {
#if ONFLYEVAL==0
            int f=255;
            pushToEvalStack(3, (void *)&(v.var_id), &stack, stack_size, stack_index);
            pushToEvalStack(2, (void *)&f, &stack, stack_size, stack_index);
#else
            double d=0.0;
            if(_getVarVal(v.var_id, mea_eval_userdata, &d) < 0) {
               *newptr=p;
               *err=10;
               return -1;
            }
            else
               pushToEvalStack(1, (void *)&d, &stack, stack_size, stack_index);
#endif
         }
         else if(ret==FUNCTION_T) {
            s=p;
            mea_eval_getSpace(s, &p);
            
            s=p;
            if(*s!='(') {
               *newptr=p;
               *err=6; // parenthèse ouvrante attendu
               return -1;
            }
            else {
               ++s;
               ++(*lvl);
               if(_evalCalcN(s, &p, lvl, stack, stack_size, stack_index, err)<0)
                  return -1;
               --(*lvl);

               s=p;
               mea_eval_getSpace(s,&p);

               s=p;
               if(*s!=')') {
                  *newptr=p;
                  *err=7; // parenthèse fermante attendu (fonction)
                  return -1;
               }
               ++p;

               s=p;
               //int op=v.fn_id;
               if(operators_index==-1)
                  operators[++operators_index]=v.fn_id;
               else {
                  if(mea_eval_operatorPriorityCmpN(v.fn_id, operators[operators_index])<=0) {
#if ONFLYEVAL==0
                     pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
                     if(mea_eval_calcOperationN(operators[operators_index], stack, stack_size, stack_index) < 0) {
                        *newptr=p;
                        *err=11;
                        return -1;
                     }
#endif
                     operators[operators_index]=v.fn_id;
                  }
                  else
                     operators[++operators_index]=v.fn_id;
               }
            }
         }
         else {
            *newptr=p;
            *err=4; // pas d'opérande
            return -1;
         }
      }
      
      s=p;
      mea_eval_getSpace(s, &p);

      s=p;
      int op=mea_eval_getOperatorN(s, &p);
      if(op!=-1) {
         if(operators_index==-1)
            operators[++operators_index]=op;
         else {
            if(mea_eval_operatorPriorityCmpN(op, operators[operators_index])<=0) {
#if ONFLYEVAL==0
               pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
               if(mea_eval_calcOperationN(operators[operators_index], stack, stack_size, stack_index) < 0) {
                  *newptr=p;
                  *err=11;
                  return -1;
               }
#endif
               operators[operators_index]=op;
            }
            else
               operators[++operators_index]=op;
         }
      }
      else if((*s == 0) || (*s == ')' && *lvl > 0)) {
         break;
      }
      else {
         *newptr=p;
         if(*s==')')
            *err=8;
         else
            *err=9;
         return -1;
      }
   }
   while(1);
   
   *newptr=p;
#if ONFLYEVAL==0
   // flush tous les opérateurs sur la pile principale
   for(;operators_index>=0;--operators_index)
      pushToEvalStack(2, (void *)&(operators[operators_index]), &stack, stack_size, stack_index);
#else
   // évaluation des opérations restantes
   for(;operators_index>=0;--operators_index)
      mea_eval_calcOperationN(operators[operators_index], stack, stack_size, stack_index);
#endif
   return 0;
}


int16_t mea_eval_setGetVarCallBacks(getVarId_f fid, getVarVal_f fval, void *userdata)
{
   _getVarVal=fval;
   _getVarId=fid;
   mea_eval_userdata=userdata;
   
   return 0;
} 


#define DEFAULT_STACK_SIZE 80

#if ONFLYEVAL==0
#define EXEC_STACK_SIZE 50
struct mea_eval_stack_s *mea_eval_buildStack_alloc(char *str, char **p, int16_t *err, int32_t *stack_ptr)
{
   int16_t lvl=0;
   struct mea_eval_stack_s *stack, *tmp;
   int32_t stack_index=-1;
   int stack_size = DEFAULT_STACK_SIZE;

   stack = (struct mea_eval_stack_s *)malloc(stack_size * (sizeof(struct mea_eval_stack_s)));
   if(stack==NULL) {
      *err = -1;
      return NULL;
   }

   int ret=_evalCalcN(str, p, &lvl, stack, &stack_size, &stack_index, err);
   if(ret < 0)
      return NULL;

   tmp=stack;
   stack=realloc(stack, (stack_index+1)*(sizeof(struct mea_eval_stack_s)));
   if(stack==NULL) {
      if(tmp)
         free(tmp);
      return NULL;
   }

   *stack_ptr=stack_index;

   return stack;
}

#ifdef DEBUG
static void displayStack(struct mea_eval_stack_s *stack, int32_t stack_ptr)
{
   for(int i=0; i<=stack_ptr; ++i) {
      if(stack[i].type==1) {
         fprintf(stderr,"v = %f\n", stack[i].val.value);
      }
      else if(stack[i].type==2) {
         if(stack[i].val.op<128)
            fprintf(stderr,"op = %c\n", stack[i].val.op);
         else
            fprintf(stderr,"f = %d\n", stack[i].val.op);
      }
      else
         fprintf(stderr,"id = %d\n", stack[i].val.id);
   }
}
#endif

static int16_t mea_eval_calcn(struct mea_eval_stack_s *stack, int32_t stack_ptr, double *r)
{
   struct mea_eval_stack_s exec_stack[EXEC_STACK_SIZE];
   int32_t exec_stack_index=-1;
   double d, d1, d2;

   for(int i=0;i<=stack_ptr;++i) {
      if(stack[i].type==1)
         exec_stack[++exec_stack_index].val.value = stack[i].val.value;
      else if(stack[i].type==2) {
         int op = stack[i].val.op;
         if(op<128) {
            d2=exec_stack[exec_stack_index--].val.value;
            d1=exec_stack[exec_stack_index--].val.value;
            if(mea_eval_doOperationN(&d, d1, op, d2)<0)
               return -1;
         }
         else {
            if(op==255) {
               int16_t var_id=exec_stack[exec_stack_index--].val.id;
               if(_getVarVal(var_id, mea_eval_userdata, &d)<0)
                  return -1;
            }
            else {
               d1=exec_stack[exec_stack_index--].val.value;
               if(mea_eval_callFunction(&d, op, d1)<0)
                  return -1;
            }
         }
         exec_stack[++exec_stack_index].val.value=d;
      }
      else if(stack[i].type==3)
         exec_stack[++exec_stack_index].val.id = stack[i].val.id;
      else
         return -1;

        if(exec_stack_index >= EXEC_STACK_SIZE-1) {
           fprintf(stderr,"execution stack overflow\n");
           return -1;
        }
   }

   if(exec_stack_index!=0) {
      *r=0.0;
      return -1;
   }
   else {
      *r=exec_stack[0].val.value;
      return 0;
   }

   return 0;
}
#else
static int16_t mea_eval_calcn(char *str, char **p, double *r, int16_t *err)
{
   int16_t lvl=0;
   struct mea_eval_stack_s *stack;
   int32_t stack_index=-1;
   int32_t stack_size = DEFAULT_STACK_SIZE;
 
   stack = (struct mea_eval_stack_s *)malloc(stack_size * (sizeof(struct mea_eval_stack_s)));
   if(stack==NULL)
      return -1;

   int16_t ret=_evalCalcN(str, p, &lvl, stack, &stack_size, &stack_index, err);

   if(ret < 0) {
      free(stack);
      return -1;
   }
   if(stack_index!=0 && stack[0].type!=1 && *p != 0) {
      *r=0.0;
      free(stack);
      return -1;
   }
   else {
      *r=stack[0].val.value;
      free(stack);
      return 0;
   }
}
#endif


#if ONFLYEVAL==0
int16_t mea_eval_calc_numeric(char *str, double *r)
{
   char *p;
   int16_t err;
   double d = 0.0;

   int32_t stack_ptr=0;

   struct mea_eval_stack_s *stack=mea_eval_buildStack_alloc(str, &p, &err, &stack_ptr);
   if(stack == NULL)
      return -1;

   int ret=mea_eval_calcn(stack, stack_ptr, &d);

   *r=d;

   free(stack);

   return ret;
}
#else
int16_t mea_eval_calc_numeric(char *str, double *r)
{
   char *p;
   int16_t err;
   double d;

   int ret=mea_eval_calcn(str, &p, &d, &err);

   *r=d;

   return ret;
}
#endif


#if ONFLYEVAL==0

struct mea_eval_stack_cache_s {
   char *expr;
   struct mea_eval_stack_s *stack;
   int stack_ptr;
   int err;
   UT_hash_handle hh;
};

struct mea_eval_stack_cache_s *mea_eval_stacks_cache = NULL;


struct mea_eval_stack_s *mea_eval_getStackFromCache(char *expr, int *stack_ptr, int16_t *err)
{
   *err=0;
   struct mea_eval_stack_cache_s *e = NULL;
   HASH_FIND_STR(mea_eval_stacks_cache, expr, e);

   if(e) {
      *stack_ptr = e->stack_ptr;
      *err = e->err;
      return e->stack;
   }

   return NULL;
}


struct mea_eval_stack_s *mea_eval_addStackToCache(char *expr, struct mea_eval_stack_s *stack, int stack_ptr, int16_t err)
{
   struct mea_eval_stack_cache_s *e = NULL;
   HASH_FIND_STR(mea_eval_stacks_cache, expr, e);
   if(e) {
      if(e->stack)
         free(e->stack);
      e->stack = stack;
      e->stack_ptr = stack_ptr;
   }
   else {
      int l_expr = (int)(strlen(expr));

      e = (struct mea_eval_stack_cache_s *)malloc(sizeof(struct mea_eval_stack_cache_s));
      if(e == NULL)
         return NULL;
      e->expr = (char *)malloc(l_expr+1);
      if(e->expr == NULL) {
         free(e);
         e=NULL;
         return NULL;
      } 
      strcpy(e->expr, expr);
      e->stack = stack;
      e->stack_ptr = stack_ptr;
      e->err = err;

      HASH_ADD_STR(mea_eval_stacks_cache, expr, e);
   }

   return stack;
}


void mea_eval_clean_stack_cache()
{
   if(mea_eval_stacks_cache) {
      struct mea_eval_stack_cache_s  *current, *tmp;

      HASH_ITER(hh, mea_eval_stacks_cache, current, tmp) {
         HASH_DEL(mea_eval_stacks_cache, current);
         if(current->expr)
            free(current->expr);
         if(current->stack)
            free(current->stack);
         free(current);
      }
      mea_eval_stacks_cache = NULL;
   }
}


int mea_eval_calc_numeric_by_cache(char *expr, double *d)
{
   char *p = NULL;
   int16_t err=0;

   int stack_ptr=0;
   struct mea_eval_stack_s *stack = NULL;

   stack = mea_eval_getStackFromCache(expr, &stack_ptr, &err);
   if(err == 0 && stack == NULL) {
      stack=mea_eval_buildStack_alloc(expr, &p, &err, &stack_ptr);
      if(stack == NULL && err == -1) {
         fprintf(stderr,"get stack error (%d) at \"%s\"\n", err, p);
         return -1;
      }

      if(mea_eval_addStackToCache(expr, stack, stack_ptr, err)==NULL) {
         if(stack)
            free(stack);
         stack=NULL;
         return -1;
      }
   }

   if(!stack) {
      return -1;
   }

   int ret = mea_eval_calcn(stack, stack_ptr, d);
   return ret;
}
#endif

#ifdef MEA_EVAL_MODULE_TEST
double millis()
{
   struct timeval te;
   gettimeofday(&te, NULL);

   double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0;

   return milliseconds;
}


int16_t myGetVarId(char *str, void *userdata, int16_t *id)
{
   *id=1234;

   return 0;
}

int16_t myGetVarVal(int16_t id, void *userdata, double *d)
{
   *d=5678;

   return 0;
}

int main(int argc, char *argv[])
{
   char *expr = "int(#2 + #1 + {tata} * #12) - #123.4 * (#567.8 + #10) * (#1 / (#2 + #3))";

   char *p;
   int16_t err;
   double d;
  
 
   mea_eval_setGetVarCallBacks(&myGetVarId, &myGetVarVal, NULL);

   double t0=millis();

#if ONFLYEVAL==0
//   int stack_ptr=0;
//   struct mea_eval_stack_s *stack=NULL;

   for(int i=0;i<400000;++i)
      mea_eval_calc_numeric_by_cache(expr, &d);

   mea_eval_clean_stack_cache();

#else
   for(int i=0;i<400000;++i) {
      mea_eval_calc_numeric(expr, &d);
   }
#endif
   fprintf(stderr, "execution time : %5.2f ms\n",millis()-t0);
   fprintf(stderr,"#%f\n", d);
}
#endif

