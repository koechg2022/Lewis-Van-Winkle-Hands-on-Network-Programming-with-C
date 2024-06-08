

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <w2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")

    #define valid_socket(the_socket) (the_socket != INVALID_SOCKET)
    #define close_socket(the_socket) (closesocket(the_socket))
    #define get_socket_errno() (WSAGetLastError())
    #define has_keyboard_input(socket_set) (_kbhit())

    #define socket_type SOCKET

#else

    #define unix_os

    #if not defined(_SYS_TYPES_H_)
        #include <sys/types.h>
    #endif

    #if not defined(_SYS_SOCKET_H_)
        #include <sys/socket.h>
    #endif

    #if not defined(_NETDB_H_)
        #include <netdb.h>
    #endif

    #if not defined(_NETINET_IN_H_)
        #include <netinet/in.h>
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


    #define valid_socket(the_socket) (the_socket >= 0)
    #define close_socket(the_socket) (close(the_socket))
    #define get_socket_errno() (errno)
    #define has_keyboard_input(socket_set) (FD_ISSET(STDIN_FILENO, &socket_set))

    #define socket_type int

#endif

#if not defined(_LIBCPP_STDIO_H)
    #include <stdio.h>
#endif

#if not defined(_LIBCPP_STRING_H)
    #include <string.h>
#endif


#define MACHINE_ADDR_RET_FAIL 2
#define SOCKET_CREAT_FAIL 3
#define BIND_SOCKET_FAIL 4
#define LISTEN_SOCKET_FAIL 5
#define SEL_SOCKET_FAIL 6
#define NEW_CONNECT_FAIL 7

#define buffer_size 100
#define kilo_byte 1024
#define four_kbytes 4096



bool is_caps(const char c);
bool is_lower(const char c);

bool same_char(const char a, const char b, bool ignore_case = true);
bool same_string(const char* first, const char* second, bool ignore_case = true);

char to_caps(const char c);
char to_lower(const char c);

void capitalize(char* the_string);


int initialize();

int clean_up();

int main(void) {
    if (initialize()) {
        return 1;
    }
    const char* port_val = "8080";
    const int listen_limit = 10;
    printf("Configuring local address...\n");
    struct addrinfo hints, *this_machine;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port_val, &hints, &this_machine)) {
        fprintf(stderr, "Failed to retrieve this machine's socket information. Error %d\n", get_socket_errno());
        return MACHINE_ADDR_RET_FAIL;
    }

    printf("Creating socket...\n");
    socket_type listening_socket = socket(this_machine->ai_family, this_machine->ai_socktype, this_machine->ai_protocol);
    if (!valid_socket(listening_socket)) {
        fprintf(stderr, "Failed to create the listening socket on this server. Error %d\n", get_socket_errno());
        freeaddrinfo(this_machine);
        return SOCKET_CREAT_FAIL;
    }

    printf("Binding the socket...\n");
    if (bind(listening_socket, this_machine->ai_addr, this_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to bind the newly created listening socket for this server. Error %d\n", get_socket_errno());
        close_socket(listening_socket);
        freeaddrinfo(this_machine);
        return BIND_SOCKET_FAIL;
    }

    freeaddrinfo(this_machine);


    printf("Listening...\n");
    if (listen(listening_socket, listen_limit)) {
        fprintf(stderr, "Failed to start listening on the listening socket of this server. Error %d\n", get_socket_errno());
        close_socket(listening_socket);
        return LISTEN_SOCKET_FAIL;
    }

    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(listening_socket, &master_set);
    socket_type max_socket = listening_socket;

    char addr_buff[buffer_size], serv_buff[buffer_size];
    getnameinfo(this_machine->ai_addr, this_machine->ai_addrlen, addr_buff, buffer_size, serv_buff, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV);
    printf("Connect to this machine with:\t%s and service %s\n", addr_buff, serv_buff);
    getnameinfo(this_machine->ai_addr, this_machine->ai_addrlen, addr_buff, buffer_size, serv_buff, buffer_size, NI_NAMEREQD);
    printf("\t%s\t:\t%s\n", addr_buff, serv_buff);

    printf("Waiting for connections...\n");
    char client_msg[four_kbytes];
    int bytes_rec, bytes_sent;

    struct sockaddr_storage new_client;
    socklen_t client_size = sizeof(new_client);
    socket_type new_socket;

    bool break_all = false;
    while (true) {
        fd_set read_set;
        read_set = master_set;
        if (select(max_socket + 1, &read_set, 0, 0, 0) < 0) {
            fprintf(stderr, "Failed to correctly select a socket. An error occured. Error %d\n", get_socket_errno());
            return SEL_SOCKET_FAIL;
        }

        socket_type this_socket;
        for (this_socket = 1; this_socket <= max_socket; this_socket = this_socket + 1) {
            
            // Is there a new incomming connection?
            if (FD_ISSET(this_socket, &read_set)) {

                // there is a new incomming connection:
                if (this_socket == listening_socket) {
                    new_socket = accept(listening_socket, (struct sockaddr*) &new_client, &client_size);

                    if (!valid_socket(new_socket)) {
                        fprintf(stderr, "Failed to accept new connection. Error %d\n", get_socket_errno());
                        return NEW_CONNECT_FAIL;
                    }

                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_socket) {
                        max_socket = new_socket;
                    }
                    
                    getnameinfo((struct sockaddr*) &new_client, client_size, addr_buff, buffer_size, serv_buff, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV);
                    printf("New connection from %s (%s)\n", addr_buff, serv_buff);

                }

                else if (has_keyboard_input(read_set)) {
                    if (!fgets(client_msg, four_kbytes, stdin)) {
                        printf("Could not understand input.\n");
                        continue;
                    }
                    if (same_string(client_msg, (char *) "exit")) {
                        break_all = true;
                        break;
                    }
                    
                }

                // There is a message from a client that is already connected
                else {
                    bytes_rec = recv(this_socket, client_msg, four_kbytes, 0);
                    if (bytes_rec < 1) {
                        FD_CLR(this_socket, &master_set);
                        close_socket(this_socket);
                        continue;
                    }
                    // make the message uppercase
                    capitalize(client_msg);
                    bytes_sent = send(this_socket, client_msg, bytes_rec, 0);
                    printf("Sent %d of %d bytes.\n", bytes_sent, bytes_rec);
                    memset(client_msg, 0, four_kbytes);
                }

            }


        }
        if (break_all) {
            for (this_socket = 1; this_socket <= max_socket; this_socket = this_socket + 1) {
                if (this_socket == STDIN_FILENO || this_socket == STDOUT_FILENO || this_socket == STDERR_FILENO) {
                    continue;
                }
                close_socket(this_socket);
            }
            break;
        }
    }

    printf("Closing listening socket...\n");
    close_socket(listening_socket);

    clean_up();
    printf("Finished. Socket is closing now.\n");
    return 0;
}


bool is_caps(const char c) {
    return (c >= 'A' && c <= 'Z');
}

bool is_lower(const char c) {
    return (c >= 'a' && c <= 'z');
}

char to_caps(const char c) {
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? (c + ('a' - 'A')) : c;
}

bool same_char(const char a, const char b, bool ignore_case) {
    return (ignore_case) ? (to_caps(a) == to_caps(b)) : a == b;
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

void capitalize(char* the_string) {
    unsigned long index;
    for (index = 0; the_string[index] != '\0'; index = index + 1) {
        the_string[index] = to_caps(the_string[index]);
    }
}


int initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Failed to initialize crap network.\n");
            return 1;
        }
    #endif
    return 0;
}


int clean_up() {
    #if defined(crap_os)
        WSACleanup();
    #endif
    return 0;
}