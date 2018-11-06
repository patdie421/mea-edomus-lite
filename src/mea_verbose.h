#ifndef __mea_verbose_h
#define __mea_verbose_h

#include <pthread.h>
#include <stdarg.h>
#include <inttypes.h>

#ifndef DEBUGFLAG
#define DEBUGFLAG 1
#endif

#define ERROR_STR        _error_str
#define INFO_STR         _info_str
#define DEBUG_STR        _debug_str
#define FATAL_ERROR_STR  _fatal_error_str
#define WARNING_STR      _warning_str
#define MALLOC_ERROR_STR _malloc_error_str

#define DEBUG_SECTION2(x) if((x))

// #define DEBUG_SECTION if(debug_msg)
#define DEBUG_SECTION DEBUG_SECTION2(DEBUGFLAG)

#define VERBOSE(v) if(v <= verbose_level)
#define MEA_STDERR stderr

extern int verbose_level;
extern int debug_msg;

extern const char *_error_str;
extern const char *_info_str;
extern const char *_debug_str;
extern const char *_fatal_error_str;
extern const char *_warning_str;
extern const char *_malloc_error_str;

#define PRINT_MALLOC_ERROR { mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR); perror(""); }

void set_verbose_level(int level);
void debug_on(void);
void debug_off(void);
int16_t debug_status(void);

void mea_log_printf(char const* fmt, ...);

int mea_rotate_open_log_file(char *name, uint16_t max_index);

#endif
