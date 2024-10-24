



#if defined(_WIN32)

    #if not defined (_WIN32_WINNT)
        #define _WIN32_WINNT
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    
    #define socket_type SOCKET

    #define valid_socket(this_socket) ((this_socket) != INVALID_SOCKET)
    #define close_socket(this_socket) closesocket(this_socket)
    #define get_socket_error() (WSAGetLastError())


#else

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>

    #define socket_type int

    #define valid_socket(this_socket) ((this_socket) >= 0)
    #define close_socket(this_socket) close(this_socket)
    #define get_socket_error() (errno)

#endif








#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>


#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#define NOT !=
#define buffer_size 100
#define two_kilo_bytes 2048
#define response_size 32768


/*

    Compile on Unix:
        clang++ [FILE_WITH_OPENSSL].c++ -o [FILE_OBJECT_NAME] -lssl -lcrypto -Wall -Wextra -std=c++23 -I/usr/local/Cellar/openssl@3/3.3.2/include -L/usr/local/Cellar/openssl@3/3.3.2/lib
        
        or 

        g++ [FILE_WITH_OPENSSL].c++ -o [FILE_OBJECT_NAME] -lcrypto

    Compile on Crap os
        clang++ [FILE_WITH_OPENSSL].c++ -o [FILE_OBJECT_NAME].exe -lcrypto

*/