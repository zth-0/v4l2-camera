#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NDEBUG
#define debug(M, ...)
#else

// In C, concatenating string can be done by having #define concat_str S1 S2
#define debug(M, ...) fprintf(stderr, "[%s][DEBUG] %s:%d: " M "\n", \
		__TIMESTAMP__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, \
		"[%s][ERROR] (%s:%d:%s errno: %s) " M "\n",__TIMESTAMP__, __FILE__, __LINE__, __FUNCTION__, \
		clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, \
        "[%s][ WARN] (%s:%d: errno: %s) " M "\n",__TIMESTAMP__, __FILE__, __LINE__, \
        clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, \
		"[%s][ INFO] (%s:%d) " M "\n",__TIMESTAMP__, __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if (!(A)) {\
		log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...) { log_err(M, ##__VA_ARGS__);\
		errno=0; goto error; }

#define check_mem(A) check((A), "Memory Error.")

#define check_debug(A, M, ...) if (!(A)) { debug(M, ##__VA_ARGS__);\
		errno=0; goto error; }

#endif
