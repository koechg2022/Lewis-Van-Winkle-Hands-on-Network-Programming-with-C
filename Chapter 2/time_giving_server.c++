
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    
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

#else
    
    #define unix_os

    #if not defined(_SYS_SOCKET_H_)
        #include <sys/socket.h>
    #endif

    #if not defined(_SYS_TYPES_H_)
        #include <sys/types.h>
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

    printf("Configuring the local address...\n");
    struct addrinfo* this_machine, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &this_machine)) {
        fprintf(stderr, "Error retrieving this machine's connection information. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    // this_machine now has the information for the current machine
    printf("Creating the listening socket.\n");
    socket_type listening_socket = socket(this_machine->ai_family, this_machine->ai_socktype, this_machine->ai_protocol);

    if (!valid_socket(listening_socket)) {
        fprintf(stderr, "Failed to create the listening socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    // bind the socket.
    printf("Binding the socket...\n");
    if (bind(listening_socket, this_machine->ai_addr, this_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to bind the listening socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }
    // Done with this machine's information.


    // Start listening.
    printf("Socket is bound. Gonna start listening on the bound socket...\n");
    char addr_buffer[buffer_size], service_buffer[buffer_size];
    getnameinfo(this_machine->ai_addr, this_machine->ai_addrlen, addr_buffer, buffer_size, service_buffer, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV);
    printf("Connect to this machine with\n\thttp://%s:%s\n", addr_buffer, service_buffer);
    freeaddrinfo(this_machine);
    if (listen(listening_socket, listening_count)) {
        fprintf(stderr, "Failed to start listening on the socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }



    // Socket is set up to listen. Now let's blocka nd accept incomming connections
    struct sockaddr_storage client_address;
    socklen_t client_size = sizeof(client_address);

    socket_type client_socket = accept(listening_socket, (struct sockaddr*) &client_address, &client_size);

    if (!valid_socket(client_socket)) {
        fprintf(stderr, "Something went wrong when accepting the new client socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    // Can now communicate with the other machine.

    getnameinfo((struct sockaddr*) &client_address, client_size, addr_buffer, buffer_size, service_buffer, buffer_size, NI_NAMEREQD);
    printf("%s\t", addr_buffer);
    getnameinfo((struct sockaddr*) &client_address, client_size, addr_buffer, buffer_size, 0, 0, NI_NUMERICHOST);
    printf("%s\n%s\t", addr_buffer, service_buffer);
    getnameinfo((struct sockaddr*) &client_address, client_size, 0, 0, service_buffer, buffer_size, NI_NUMERICHOST);
    printf("%s\n", service_buffer);

    char received[kilo_byte];
    int bytes_received = recv(client_socket, received, kilo_byte, 0);
    printf("Received %d bytes\n\t%.*s\n", bytes_received, bytes_received, received);

    printf("Sending response...\n");
    const char* response = 
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain \r\n\r\n"
        "Local Time on this machine is: ";
    unsigned long str_len = string_length(response);
    int bytes_sent = send(client_socket, response, str_len, 0);
    printf("Sent %d bytes of %lu bytes (for header)\n", bytes_sent, str_len);
    const char* this_time = get_current_time();
    str_len = string_length(this_time);
    bytes_sent = send(client_socket, this_time, str_len, 0);
    printf("Sent %d bytes of %lu bytes (For the current time)\n", bytes_sent, str_len);

    printf("Closing connection...\n");
    close_socket(client_socket);
    close_socket(listening_socket);
    clean_up();
    printf("Finished...\n");
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
    return (ignore_case) ? (to_caps(a) == to_caps(b)) : (a == b);
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
    return (is_lower(c)) ? c - ('a' - 'A') : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? c + ('a' - 'A') : c;
}


void initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Yo... this crappy windows machine could not initialize the network. It SUCKS!\n");
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