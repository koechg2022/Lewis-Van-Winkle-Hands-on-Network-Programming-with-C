#if defined(_WIN16) || defined(_WIN23) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define _CRT_SECURE_NO_WARNINGS
    
    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    #define crap_os

    #define valid_socket(this_socket) (this_socket != INVALID_SOCKET)
    #define close_socket(this_socket) (closesocket(this_socket))
    #define get_socket_errno() (WSAGetLastError())

    #if not defined(IPV6_V6ONLY)
        #define IPv6_V6ONLY 27
    #endif

    #define socket_type SOCKET

#else

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>

    #define unix_os

    #define valid_socket(this_socket) (this_socket >= 0)
    #define close_socket(this_socket) (close(this_socket))
    #define get_socket_errno() (errno)

    #define socket_type int

#endif



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>




#define buffer_size 100
#define kilo_byte 1024



const int listening_count = 10;

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

const char* get_current_time();



int main(int len, char** args) {

    char* port = (char *) "8080";
    if (len == 2) {
        if (all_nums(args[1])) {
            port = args[1];
        }
    }

    initialize();

    // Initialize the address
    printf("Configuring the local address\n");
    struct addrinfo* this_machine, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &this_machine)) {
        fprintf(stderr, "Failed to configuring the local address. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    printf("Creating the listening socket...\n");
    socket_type listening_socket = socket(this_machine->ai_family, this_machine->ai_socktype, this_machine->ai_protocol);

    if (!valid_socket(listening_socket)) {
        fprintf(stderr, "Failed to create the listening socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    printf("Configuring the listening socket to work with both IPV4 and IPV6...\n");
    #if defined(unix_os)
        int option = 0;
        if (setsockopt(listening_socket, IPPROTO_IPV6, IPV6_V6ONLY, (void*) &option, sizeof(option))) {
            fprintf(stderr, "Failed to configure the listening socket to work with both IPV4 and IPV6. Error %d\n", get_socket_errno());
            exit(EXIT_FAILURE);
        }
    #else
        char option[buffer_size];
        option[0] = 0;
        if (setsockopt(listening_socket, IPPROTO_IPV6, IPV6_V6ONLY, option, buffer_size)) {
            fprintf(stderr, "Failed to configure the listening socket to work with both IPV4 and IPV6. Error %d\n", get_socket_errno());
            exit(EXIT_FAILURE);
        }
    #endif


    printf("Binding the listening socket to the local address...\n");
    if (bind(listening_socket, this_machine->ai_addr, this_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to bind the listening socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(this_machine);

    printf("Setting the socket to listen...\n");
    if (listen(listening_socket, listening_count)) {
        fprintf(stderr, "Failed to set the socket to listen.\n");
        exit(EXIT_FAILURE);
    }


    printf("Waiting for connection...\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    socket_type client_socket = accept(listening_socket, (struct sockaddr*) &client_address, &client_len);

    if (!valid_socket(client_socket)) {
        fprintf(stderr, "Failed to accept an incomming connection.\n");
        exit(EXIT_FAILURE);
    }


    printf("Client is connected...\n");
    char address_buffer[buffer_size], service[buffer_size];
    getnameinfo((struct sockaddr*) &client_address, client_len, address_buffer, buffer_size, service, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV);
    printf("Connection established with client %s on service %s\n", address_buffer, service);


    printf("Reading request...\n");
    char request[kilo_byte];
    int bytes_received = recv(client_socket, request, kilo_byte, 0);
    printf("Bytes received %d.\n\t%.*s\n", bytes_received, bytes_received, request);


    printf("Sending response...\n");

    const char* response = 
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n\r\n"
    "Local time is: ";
    unsigned long resp_len = string_length(response);
    int bytes_sent = send(client_socket, response, resp_len, 0);
    printf("Sent %d of %lu bytes.\n", bytes_sent, resp_len);
    const char* current_time = get_current_time();
    resp_len = string_length(current_time);
    bytes_sent = send(client_socket, current_time, resp_len, 0);

    printf("Sent response. Closing connection...\n");
    close_socket(client_socket);

    printf("Closing listening socket...\n");
    close_socket(listening_socket);

    printf("Cleaning up...\n");
    clean_up();

    printf("Finishing up... Goodbye.\n");
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
    return (first[index] == '\0' && second[index] == '\0');
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
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? (c + ('a' - 'A')) : c;
}

void initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Could not initialize the network interface of this crappy Operating System,\n");
            exit(EXIT_FAILURE);
        }
    #endif
}

void clean_up() {
    #if defined(crap_os)
        WSACleanup();
    #endif
}

const char* get_current_time() {
    time_t raw_time;
    struct tm* time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);

    char* the_answer = asctime(time_info);
    int index;
    for (index = 0; the_answer[index] != '\0'; index = index + 1);
    the_answer[index - 1] = '\0';
    return the_answer;
}