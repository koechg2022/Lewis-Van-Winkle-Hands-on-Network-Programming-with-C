

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <conio.h>

    #pragma comment(lib, "ws2_32.lib")

    #define socket_type SOCKET

    #define valid_socket(the_socket) (the_socket != INVALID_SOCKET)
    #define close_socket(the_socket) (closesocket(the_socket))
    #define get_socket_errno() (WSAGetLastError())
    #define is_keyboard_input(the_set) (_kbhit())
    #define get_return_key() ((char *) "enter")

#else

    #define unix_os

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>

    #define socket_type int
    
    #define valid_socket(the_socket) (the_socket >= 0)
    #define close_socket(the_socket) (close(the_socket))
    #define get_socket_errno() (errno)
    #define is_keyboard_input(the_set) (FD_ISSET(0, &the_set))

    #if defined(__APPLE__)
        #define get_return_key() ((char *) "return")
    #else
        #define get_return_key() ((char *) "enter")
    #endif

#endif

#define buffer_size 100
#define kilo_byte 1024
#define msg_buf_size 4096



#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool is_caps(const char c);

bool is_lower(const char c);

bool is_letter(const char c);

bool is_number(const char c);

bool same_char(const char a, const char b, bool ignore_case = true);

bool same_string(const char* first, const char* second, bool ignore_case = true);

bool all_nums(const char* the_string);

unsigned long string_length(const char* the_string);

char to_caps(const char c);

char to_lower(const char c);

void initialize();

void clean_up();


int main(int len, char** args) {
    initialize();

    if (len < 3) {
        fprintf(stderr, "Usage : %s <hostname> <port>\n", args[0]);
        exit(EXIT_FAILURE);
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints, *remote_machine;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(args[1], args[2], &hints, &remote_machine)) {
        fprintf(stderr, "Failed to retrieve address information for \"%s\". Error %d\n", args[1], get_socket_errno());
        exit(EXIT_FAILURE);
    }


    printf("Remote address is :\n");
    char remote_address[buffer_size], remote_service[buffer_size];
    
    getnameinfo(remote_machine->ai_addr, remote_machine->ai_addrlen, remote_address, buffer_size, remote_service, buffer_size, NI_NUMERICHOST);
    printf("\t%s\t:\t%s\n", remote_address, remote_service);

    printf("Now to create the socket...\n");
    socket_type remote_socket = socket(remote_machine->ai_family, remote_machine->ai_socktype, remote_machine->ai_protocol);

    if (!valid_socket(remote_socket)) {
        fprintf(stderr, "Failed to create a listening socket to \"%s\". Error %d\n", remote_address, get_socket_errno());
        exit(EXIT_FAILURE);
    }


    printf("Socket is created. Now to connect it...\n");
    if (connect(remote_socket, remote_machine->ai_addr, remote_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to connect to remote machine \"%s\". Error %d\n", remote_address, get_socket_errno());
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(remote_machine); // Don't need this after connecting to the remote machine.

    printf("Connected.\n");
    printf("To send data, enter text followed by %s.\n", get_return_key());

    while (true) {

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(remote_socket, &reads);

        #if defined(unix_os)
            FD_SET(0, &reads);
        #endif

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(remote_socket + 1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "Failed to select a remote socket. Error %d\n", get_socket_errno());
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(remote_socket, &reads)) {
            char from_remote[msg_buf_size];
            int bytes_received = recv(remote_socket, from_remote, msg_buf_size, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            printf("Received %d bytes:\n\t\"%.*s\"\n", bytes_received, bytes_received, from_remote);
        }

        if (is_keyboard_input(reads)) {
            char to_send[msg_buf_size];
            if (!fgets(to_send, msg_buf_size, stdin)) {
                break;
            }
            printf("Sending %s...\n", to_send);
            int bytes_sent = send(remote_socket, to_send, string_length(to_send), 0);
            printf("Sent %d bytes.\n", bytes_sent);
        }


    } // while loop end.

    close_socket(remote_socket);
    clean_up();
    printf("Finished.\n");
    return 0;
}

bool is_caps(const char c) {
    return (c >= 'A' && c <= 'Z');
}

bool is_lower(const char c) {
    return (c >= 'a' && c <= 'z');
}

bool is_letter(const char c) {
    return (is_caps(c) || is_lower(c));
}

bool is_number(const char c) {
    return (c >= '0' && c <= '9');
}

bool same_char(const char a, const char b, bool ignore_case) {
    return (ignore_case) ? to_caps(a) == to_caps(b) : a == b;
}

bool same_string(const char* first, const char* second, bool ignore_case) {
    unsigned long index;
    for (index = 0; first[index] != '\0' && second[index] != '\0'; index = index + 1) {
        if (!same_char(first[index], second[index], ignore_case)) {
            return false;
        }
    }
    return first[index] == '\0' && second[index] == '\0';
}

bool all_nums(const char* the_string) {
    unsigned long index;
    for (index = 0; the_string[index] != '\0'; index = index + 1) {
        if (!is_number(the_string[index])) {
            return false;
        }
    }
    return true;
}

unsigned long string_length(const char* the_string) {
    unsigned long the_answer;
    for (the_answer = 0; the_string[the_answer] != '\0'; the_answer = the_answer + 1);
    return the_answer;
}

char to_caps(const char c) {
    return (is_lower(c)) ? c - ('a' - 'A') : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? c + ('a' - 'A') : c;
}

void initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Failed to initialize this crappy operating system.\n");
            exit(EXIT_FAILURE);
        }
    #endif
}

void clean_up() {
    #if defined(crap_os)
        WSACleanup();
    #endif
}

