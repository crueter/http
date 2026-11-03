module;

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define closesocket(s) close(s)

#elif defined(WIN32_LEAN_AND_MEAN) || defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#error "unsupported platform"
#endif

// #ifndef REQUESTS_NO_TLS

// #include <openssl/ssl.h>
// #include <openssl/pem.h>
// #include <openssl/x509.h>
// #include <openssl/x509_vfy.h>

// typedef SSL_CTX TLS_CTX;
// typedef SSL TLS_CONN;
// typedef X509 TLS_CERT;

// #endif // #ifndef REQUESTS_NO_TLS

#define REQUESTS_RECV_BUFSIZE 1024 * 32
#define STATIC_STRSIZE(str) (sizeof(str) - 1)
#define MIN(x, y) ((x < y) ? x : y)
#define OPTION(s, m) (s && (s)->m)

export module http;
export import :socket;

export namespace http
{

}