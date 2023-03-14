#ifndef MYY_LOG_H
#define MYY_LOG_H 1

#include <string.h>

#if defined(DEBUG)
#if defined(__ANDROID__)

#include <android/log.h>
#define LOG_ERROR(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-insanity", __VA_ARGS__))
#define LOG_ERRNO(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-insanity", "Error : %s\n", strerror(errno))); ((void)__android_log_print(ANDROID_LOG_ERROR, "native-insanity", __VA_ARGS__)

#else

#include <stdio.h>
#define LOG(...) fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERRNO(...)  fprintf(stderr, "Error : %s\n", strerror(errno)); fprintf(stderr, __VA_ARGS__)

#endif
#else // DEBUG
#include <stdio.h>
#define LOG(...) fprintf(stdout, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERRNO(...)  fprintf(stderr, "Error : %s\n", strerror(errno)); fprintf(stderr, __VA_ARGS__)
// #define LOG(...)
// #define LOG_ERROR(...)
// #define LOG_ERRNO(...)
#endif // DEBUG
#endif
