//
//  mea_eval.h
//
//  Created by Patrice DIETSCH on 10/12/15.
//
//
#include <inttypes.h>

#define ONFLYEVAL 0

typedef int16_t (*getVarVal_f)(int16_t id, void *userdata, double *d);
typedef int16_t (*getVarId_f)(char *str, void *userdata, int16_t *id);

struct mea_eval_stack_s {
   char type;
   union {
      int16_t op;
      double value;
      int16_t id;
   } val;
};

int16_t mea_eval_setGetVarCallBacks(getVarId_f fid, getVarVal_f fval, void *userdata);

#if ONFLYEVAL==0
struct eval_stack_s *getEvalStack(char *str, char **p, int16_t *err, int32_t *stack_ptr);
int16_t evalCalc(struct eval_stack_s *stack, int32_t stack_ptr, double *r);
void mea_eval_clean_stack_cache(void);
int mea_eval_calc_numeric_by_cache(char *expr, double *d);
#else
int16_t evalCalc(char *str, char **p, double *r, int16_t *err);
#endif
int16_t calcn(char *str, double *r);
