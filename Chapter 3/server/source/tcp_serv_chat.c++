

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <w2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")

    #define valid_socket(the_socket) (the_socket != IVNALID_SOCKET)
    #define close_socket(the_socket) (closesocket(the_socket))
    #define get_socket_errno() (WSAGetLastError())
    #define has_keyboard_input(socket_set) (_kbhit())


    #define socket_type SOCKET

#else

#endif