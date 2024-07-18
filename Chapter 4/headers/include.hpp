

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    #define _CRT_SECURE_NO_WARNINGS
    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    #pragma comment(lib, "ws2_32.lib")


    #define valid_socket(sock_no) (sock_no != INVALID_SOCKET)
    #define close_socket(sock_no) (closesocket(sock_no))
    #define get_socket_errno() (WSAGetLastError())
    #define socket_type SOCKET

    bool is_init = false;

#else
    
    #define unix_os

    #if not defined(_SYS_TYPES_H_)
        #include <sys/types.h>
    #endif

    #if not defined(_SYS_SOCKET_H_)
        #include <sys/socket.h>
    #endif


    #if not defined(_NETINET_IN_H_)
        #include <netinet/in.h>
    #endif

    #if not defined(_ARPA_INET_H_)
        #include <arpa/inet.h>
    #endif

    #if not defined(_NETDB_H_)
        #include <netdb.h>
    #endif

    #if not defined(_UNISTD_H_)
        #include <unistd.h>
    #endif

    #if not defined(_LIBCPP_ERRNO_H)
        #include <errno.h>
    #endif

    #define valid_socket(sock_no) (sock_no >= 0)
    #define close_socket(sock_no) (close(sock_no))
    #define get_socket_errno() (errno)
    #define socket_type int

    bool is_init = true;


#endif

#include <time.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>




#define buffer_size 100
#define kilo_byte 1024


bool is_caps(const char c);

bool is_lower(const char c);

bool is_letter(const char c);

bool is_number(const char c);

bool same_char(const char a, const char b, bool ignore_case = true);

bool same_string(const char* first, const char* second, bool ignore_case = true, const char terminate = '\0');

char to_caps(const char c);

char to_lower(const char c);








bool is_initialized();


bool initialize();
bool uninitialize();


int udp_client(const char* client, const char* port = "8080");
int udp_recvfrom(const char* from_host, const char* port = "8080");
int udp_sendto(const char* sendto_host, const char* port = "8080");
int upd_serve_toupper_simple(const char* client_host, const char* port = "8080");
int upd_serve_to_upper(const char* client_host, const char* port = "8080");