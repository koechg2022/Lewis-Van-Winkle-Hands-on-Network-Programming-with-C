

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")

    #define socket_type SOCKET
    #define valid_socket(the_socket) (the_socket != INVALID_SOCKET)
    #define get_socket_errno() (WSAGetLastError())
    #define close_socket(the_socket) (closesocket(the_socket))
    #define is_keyboard_input(socket_set) (_kbhit())


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

    #if not defined(_NETDB_H_)
        #include <netdb.h>
    #endif

    #if not defined(_ARPA_INET_H_)
        #include <arpa/inet.h>
    #endif

    #if not defined(_UNISTD_H_)
        #include <unistd.h>
    #endif
    
    #if not defined(_LIBCPP_ERRNO_H)
        #include <errno.h>
    #endif


    #define socket_type int
    #define valid_socket(the_socket) (the_socket >= 0)
    #define get_socket_errno() (errno)
    #define close_socket(the_socket) (close(the_socket))
    #define is_keyboard_input(socket_set) (FD_ISSET(0, &socket_set))


#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if not defined(default_port)
    #define default_port (char *) "8080"
#endif

#if not defined(basic_buffer)
    #define basic_buffer 100
#endif

#if not defined(kilo_byte)
    #define kilo_byte 1024
#endif

#if not defined(four_kilo_bytes)
    #define four_kilo_bytes 4096
#endif

#if not defined(listen_limit)
    #define listen_limit 10
#endif


bool is_caps(const char c);
bool is_lower(const char c);
bool is_letter(const char c);
bool is_number(const char c);
bool same_char(const char a, const char b, bool ignore_case = true);
bool same_string(const char* first, const char* second, bool ignore_case = true);
char to_caps(const char c);
char to_lower(const char c);
void capitalize(char* the_string, unsigned long lim_index);
void lowerize(char* the_string, unsigned long lim_index);
void initailize();
void clean_up();



int main(void) {
    initailize();
    struct addrinfo hints, *this_machine;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, default_port, &hints, &this_machine)) {
        fprintf(stderr, "Failed to retrieve this machine's information. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    printf("Creating socket...\n");
    socket_type listen_socket = socket(this_machine->ai_family, this_machine->ai_socktype, this_machine->ai_protocol);

    if (!valid_socket(listen_socket)) {
        fprintf(stderr, "Failed to create the listening socket. Error %d\n", listen_socket);
        exit(EXIT_FAILURE);
    }

    printf("Binding the socket to a local address...\n");
    if (bind(listen_socket, this_machine->ai_addr, this_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to bind the listening socket locally. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }
    char machine_name[basic_buffer], machine_service[basic_buffer];
    getnameinfo(this_machine->ai_addr, this_machine->ai_addrlen, machine_name, basic_buffer, machine_service, basic_buffer, NI_NUMERICHOST);
    printf("This server's information:\n\t%s\n\t\t%s\n", machine_name, machine_service);

    freeaddrinfo(this_machine);

    printf("Listening...\n");
    if (listen(listen_socket, listen_limit)) {
        fprintf(stderr, "Failed to start listening on the newly created socket. Error %d\n", get_socket_errno());
        exit(EXIT_FAILURE);
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listen_socket, &master);
    #if defined(unix_os)
        FD_SET(0, &master);
    #endif
    socket_type max_socket = listen_socket;

    printf("Waiting for connections...\n");

    while (true) {

        fd_set reads;
        reads = master;

        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "Failed to select an incomming message socket. Error %d\n", get_socket_errno());
            exit(EXIT_FAILURE);
        }

        socket_type this_socket;

        for (this_socket = 1; this_socket <= max_socket; this_socket = this_socket + 1) {
            if (FD_ISSET(this_socket, &reads)) {

                if (this_socket == listen_socket) { // Adding a new socket.
                    struct sockaddr_storage client_info;
                    socklen_t client_size = sizeof(client_info);
                    socket_type client_socket = accept(this_socket, (struct sockaddr*) &client_info, &client_size);

                    if (!valid_socket(client_socket)) {
                        fprintf(stderr, "Failed to accept from a connected client socket.\n Error %d\n", get_socket_errno());
                        exit(EXIT_FAILURE);
                    }

                    FD_SET(client_socket, &master); // FD_SET(fd, &fdset)  :   Sets the bit for the file descriptor fd in the file descriptor set fdset.

                    if (client_socket > max_socket) {
                        max_socket = client_socket;
                    }

                    char this_client_address[basic_buffer], this_client_service[basic_buffer];

                    getnameinfo((struct sockaddr*) &client_info, client_size, this_client_address, basic_buffer, this_client_service, basic_buffer, NI_NUMERICHOST);

                    printf("New connection from %s with service %s\n", this_client_address, this_client_service);

                }

                else {
                    char client_msg[four_kilo_bytes];
                    unsigned long bytes_received = recv(this_socket, client_msg, four_kilo_bytes, 0);
                    if (bytes_received < 1) {
                        FD_CLR(this_socket, &master); // FD_CLR(fd, &fdset)  :   Clears the bit for the file descriptor fd in the file descriptor set fdset.
                        close_socket(this_socket);
                        continue;
                    }

                    capitalize(client_msg, bytes_received);
                    int bytes_sent = send(this_socket, client_msg, bytes_received, 0);
                    printf("Re-sent message to client with socket %d. Sent a total of %d bytes...\n", this_socket, bytes_sent);
                    // printf("Received and upgraded to caps %lu characters of %lu characters.\n", send(this_socket, client_msg, bytes_received, 0), bytes_received);

                }

            } // FD_ISSET

        } // for index in max_socket

    } // while loop end

    printf("Closing listening socket...\n");
    close_socket(listen_socket);
    clean_up();

    printf("Finished.\n");
    return 0;
}


bool is_caps(const char c) {
    return (c >= 'A' && c <= 'Z');
}

bool is_lower(const char c) {
    return (c >= 'a' && c<= 'z');
}

bool is_letter(const char c) {
    return is_caps(c) || is_lower(c);
}

bool is_number(const char c) {
    return (c >= '0' && c <= '9');
}

bool same_char(const char a, const char b, bool ignore_case) {
    return (ignore_case) ? to_caps(a) == to_caps(b) : a == b;
}

bool same_string(const char* first, const char* second, bool ignore_case) {
    unsigned long index;
    for (index = 0; first[index != '\0' && second[index] != '\0']; index = index + 1) {
        if (!same_char(first[index], second[index], ignore_case)) {
            return false;
        }
    }
    return first[index] == '\0' && second[index] == '\0';
}

char to_caps(const char c) {
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? (c + ('a' - 'A')) : c;
}

void capitalize(char* the_string, unsigned long lim_index) {
    unsigned long index;
    for (index = 0; index < lim_index; index = index + 1) {
        the_string[index] = to_caps(the_string[index]);
    }
}

void lowerize(char* the_string, unsigned long lim_index) {
    unsigned long index;
    for (index = 0; index < lim_index; index = index + 1) {
        the_string[index] = to_lower(the_string[index]);
    }
}

void initailize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Failed to initialize crap OS network.\n");
            exit(EXIT_FAILURE);
        }
    #endif
}

void clean_up() {
    #if defined(crap_os)
        WSACleanup();
    #endif
}