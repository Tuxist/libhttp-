#if defined(_MSC_VER)
  #include <BaseTsd.h> 
  #define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
  #ifndef ssize_t  
    typedef SSIZE_T ssize_t;
    #define ssize_t ptrdiff_t
  #endif
  #define MSVC
#else
  #include <sys/types.h>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
  #ifndef EWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK
  #endif
  #define snprintf(buf,len, format,...) _snprintf_s(buf, len,len, format, __VA_ARGS__)
#endif

#define Linux

#define EVENT EPOLL

#define EVENT_HEADER epoll/epoll.h

#define BLOCKSIZE 16384

#define PATHSIZE 1024

#define MAXDEFAULTCONN 1024

#define MAXHEADERSIZE 16384

#define EPOLLWAIT 5000

#define NEWLINE "\r\n"

#define DEBUG

#define DEFAULTCONNECTTRIES 5

#define ARCH_x86_64
