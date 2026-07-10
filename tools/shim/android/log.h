/* Host-side stand-in for the NDK's <android/log.h>, so the facade and the JNI
 * bridge can be syntax-checked with a stock gcc. Never compiled into the app:
 * the real header comes from the NDK. */
#ifndef GNUBG_HOST_ANDROID_LOG_SHIM_H
#define GNUBG_HOST_ANDROID_LOG_SHIM_H
enum {
    ANDROID_LOG_VERBOSE = 2, ANDROID_LOG_DEBUG, ANDROID_LOG_INFO,
    ANDROID_LOG_WARN, ANDROID_LOG_ERROR
};
extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);
#endif
