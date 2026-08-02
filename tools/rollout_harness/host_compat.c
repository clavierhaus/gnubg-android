/* Host-side stand-ins for symbols the Android build defines in native-lib.c
 * (excluded here: it is JNI marshalling). Grows only as the linker demands. */
#include <pthread.h>
pthread_mutex_t gnubg_lock = PTHREAD_MUTEX_INITIALIZER;

/* Android logcat on the host: honest stderr, never silenced. */
#include <stdio.h>
#include <stdarg.h>
int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list ap; (void) prio;
    fprintf(stderr, "[%s] ", tag);
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr);
    return 0;
}
