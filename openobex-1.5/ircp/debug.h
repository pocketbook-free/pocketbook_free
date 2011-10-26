//#define DEBUG_TCP 1

#if IRCP_DEBUG
#define DEBUG(n, format, ...) if(n <= IRCP_DEBUG) printf("%s(): " format, __FUNCTION__ , ## __VA_ARGS__)
#else
#define DEBUG(n, format, ...)
#endif //IRCP_DEBUG
