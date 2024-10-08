#ifndef CONFIG_H_
#define CONFIG_H_

#define SCTP_MTU (1200)
#define CONFIG_MTU (1300)
#define RSA_KEY_LENGTH 1024

#ifdef ESP32
#define VIDEO_RB_DATA_LENGTH (CONFIG_MTU * 64)
#define AUDIO_RB_DATA_LENGTH (CONFIG_MTU * 64)
#define DATA_RB_DATA_LENGTH (SCTP_MTU * 128)
#else
#define HAVE_USRSCTP
#define VIDEO_RB_DATA_LENGTH (CONFIG_MTU * 256)
#define AUDIO_RB_DATA_LENGTH (CONFIG_MTU * 256)
#define DATA_RB_DATA_LENGTH (SCTP_MTU * 128)
#endif

#define AUDIO_LATENCY 20 // ms
#define KEEPALIVE_CONNCHECK 10000
#define CONFIG_IPV6 0
// default use wifi interface
#define IFR_NAME "w"

//#define LOG_LEVEL LEVEL_DEBUG
#ifndef LOG_REDIRECT
#define LOG_REDIRECT 0
#endif

#ifndef BUFFER_USE_MUTEX
#define BUFFER_USE_MUTEX  0
#endif

#endif // CONFIG_H_
