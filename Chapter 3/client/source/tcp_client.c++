


#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN__) || \
                                        defined(__TOS_WIN__) || defined(__WINDOWS__)

    #define crap_os

    #if not defined(_WIN32_WINNT)
        #define _WINN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    #pragma comment(lib, "ws2_32.lib")

    #include <conio.h>

    #define socket_type SOCKET
    #define STDIN_FILENO STD_INPUT_HANDLE
    #define STDOUT_FILENO STD_OUTPUT_HANDLE
    #define STDERR_FILENO STD_ERROR_HANDLE

    #define valid_socket(the_socket) (the_socket != INVALID_SOCKET)
    #define get_socket_errno() (WSAGetLastError())
    #define close_socket(the_socket) (closesocket(the_socket))
    #define get_return_key() ((char *) "enter")
    #define has_keyboard_input(the_socket) (_kbhit())
    #define get_socket_option(the_socket, e_code, e_code_size) (\
    getsockopt(the_socket, SOL_SOCKET, SO_ERROR, (char *) &e_code, &e_code_size) == SOCKET_ERROR \
    )
    #define is_std_socket(the_socket) (\
        the_socket == STD_INPUT_HANDLE ||\
        the_socket == STD_OUTPUT_HANDLE ||\
        the_socket == STD_ERROR_HANDLE\
    )
    #define add_input_descriptor(the_set) ()


#else
    
    #define unix_os

    #if not defined(_SYS_TYPES_H_)
        #include <sys/types.h>
    #endif

    #if not defined(_SYS_SOCKET_H_)
        #include <sys/socket.h>
    #endif

    #if not defined(_UNISTD_H_)
        #include <unistd.h>
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

    #if not defined(_LIBCPP_ERRNO_H)
        #include <errno.h>
    #endif

    
    #define socket_type int

    #define valid_socket(the_socket) (the_socket >= 0)
    #define get_socket_errno() (errno)
    #define close_socket(the_socket) (close(the_socket))
    #if defined(__APPLE__)
        #define get_return_key() ((char *) "return")
    #else
        #define get_return_key() ((char *) "enter")
    #endif
    #define has_keyboard_input(socket_set) (FD_ISSET(STDIN_FILENO, &socket_set))
    #define get_socket_option(the_socket, e_code, e_code_size) (\
    getsockopt(the_socket, SOL_SOCKET, SO_ERROR, &e_code, &e_code_size) == -1\
    )
    #define is_std_socket(the_socket) (\
        the_socket == STDIN_FILENO ||\
        the_socket == STDOUT_FILENO ||\
        the_socket == STDERR_FILENO\
    )
    
    #define add_input_descriptor(the_set) (FD_SET(STDIN_FILENO, &the_set))



#endif

#if not defined(_LIBCPP_STDIO_H)
    #include <stdio.h>
#endif

#if not defined(_LIBCPP_STRING_H)
    #include <string.h>
#endif


#define buffer_size 100
#define kilo_byte 1024
#define four_kb 4096


#define INIT_FAIL 1
#define REMOTE_RET_FAIL 2
#define SOCKET_CREAT_FAIL 3
#define GET_REMT_NAME_FAIL 4
#define CONNECT_FAIL 5
#define SELECT_FAIL 6

bool is_caps(const char c);
bool is_lower(const char c);
bool is_letter(const char c);
bool same_char(const char a, const char b, bool ignore_case = true);
bool same_string(const char* first, const char* second, bool ignore_case = true);
char to_caps(const char c);
char to_lower(const char c);


int initialize();

int clean_up(const socket_type max_socket);

int run_client(char* host, char* port);

int main(int len, char** args) {
    if (len < 3) {
        fprintf(stderr, "Usage: <%s> <hostname> <port>\n", args[0]);
        return 1;
    }
    return run_client(args[1], args[2]);
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

bool same_char(const char a, const char b, bool ignore_case) {
    return (ignore_case) ? to_caps(a) == to_caps(b) : a == b;
}

bool same_string(const char* first, const char* second, bool ignore_case) {
    unsigned long index;
    for (index = 0; first[index] != '\0' && second[index] != '\0' && same_char(first[index], second[index], ignore_case); index = index + 1);
    return first[index] == '\0' && second[index] == '\0';
}

char to_caps(const char c) {
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}

char to_lower(const char c) {
    return (is_caps(c)) ? (c + ('a' - 'A')) : c;
}

int initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Failed to initialize the network.\n");
            return 1;
        }
    #endif
    return 0;
}

int clean_up(const socket_type max_socket) {
    int error_code;
    socklen_t error_code_size = (int) sizeof(error_code);
    socket_type this_socket;
    printf("max_socket is %d\n", max_socket);
    for (this_socket = 0; this_socket <= max_socket; this_socket = this_socket + 1) {
        // don't wanna mess with std sockets at all.
        printf("In for loop. this_socket = %d\n", this_socket);
        if (is_std_socket(this_socket)) {
            continue;
        }
        if (get_socket_option(this_socket, error_code, error_code_size)) {
            continue;
        }
        printf("Closing socket %d\n", this_socket);
        close_socket(this_socket);
    }
    #if defined(crap_os)
        WSACleanup();
    #endif
    return 0;
}

int run_client(char* host, char* port) {
    if (initialize()) {
        fprintf(stderr, "Failed to initialize this crappy machine\n");
        return INIT_FAIL;
    }

    struct addrinfo hints, *remote_machine;
    socket_type remote_sock, max_socket = 0;
    char remote_addr[buffer_size], serv_buff[buffer_size], received_msg[four_kb];
    fd_set master_set, ready_set;
    int status, socket_count, received_bytes, sent_bytes;
    memset(remote_addr, 0, buffer_size);
    memset(serv_buff, 0, buffer_size);
    memset(&hints, 0, sizeof(hints));
    printf("Retrieving remote machine's address information:\n");
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(host, port, &hints, &remote_machine)) {
        fprintf(stderr, "Failed to retrieve remote machine's address information. Error %d\n", get_socket_errno());
        return REMOTE_RET_FAIL;
    }

    if ((status = getnameinfo(remote_machine->ai_addr, remote_machine->ai_addrlen, remote_addr, buffer_size, serv_buff, buffer_size, 0))) {
        fprintf(stderr, "Failed to get remote machine's name information. Error %d\n", status);
        printf("Continue with program execution? ([y or (anything else)] the '%s'):\t", get_return_key());
        if (!same_char((const char) getchar(), 'y')) {
            freeaddrinfo(remote_machine);
            return GET_REMT_NAME_FAIL;
        }
    }

    else {
        printf("Retrieved remote machine information:\n");
        printf("\tRemote Address:\t%s\n", remote_addr);
        printf("\t\tRemote Service:\t%s\n\n\n", serv_buff);
    }


    printf("Creating connection socket...\n");
    remote_sock = socket(remote_machine->ai_family, remote_machine->ai_socktype, remote_machine->ai_protocol);

    if (!valid_socket(remote_sock)) {
        fprintf(stderr, "Failed to create a socket for remote machine at \"%s\". Error %d\n", remote_addr, get_socket_errno());
        freeaddrinfo(remote_machine);
        return SOCKET_CREAT_FAIL;
    }

    printf("Successfully created the socket for the remote machine at \"%s\". Socket is %d\n", remote_addr, remote_sock);

    printf("Connecting the socket to the remote machine:\n");

    if (connect(remote_sock, remote_machine->ai_addr, remote_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to connect socket %d to the remote machine. Error %d\n", remote_sock, get_socket_errno());
        return CONNECT_FAIL;
    }

    freeaddrinfo(remote_machine);
    printf("Successfully connected socket \"%d\" with remote machine \"%s\"\n", remote_sock, remote_addr);
    printf("To send a message, enter the message followed by the '%s' key\n\n", get_return_key());
    FD_ZERO(&master_set);
    FD_SET(remote_sock, &master_set);
    add_input_descriptor(master_set);
    socket_count = 1;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    max_socket = remote_sock + 1;
    while (socket_count) {
        
        ready_set = master_set;
        if (select(max_socket, &ready_set, 0, 0, &timeout) < 0) {
            fprintf(stderr, "Failed to select the active interface to retrieve data from. Error %d\n", get_socket_errno());
            return SELECT_FAIL;
        }

        if (FD_ISSET(remote_sock, &ready_set)) {
            // There is a message from the remote machine.
            memset(received_msg, 0, four_kb);
            received_bytes = recv(remote_sock, received_msg, four_kb, 0);
            if (received_bytes < 1) {
                printf("Connection closed by peer\n");
                FD_CLR(remote_sock, &master_set);
                socket_count = socket_count - 1;
                break;
            }
            printf("Received %d bytes:\n\"%.*s\"\n", received_bytes, received_bytes, received_msg);
        }

        else if (has_keyboard_input(ready_set)) {
            // There is keyboard input.
            memset(received_msg, 0, four_kb);
            if (!fgets(received_msg, four_kb, stdin)) {
                break;
            }
            if (same_string(received_msg, (const char*) "exit")) {
                break;
            }
            size_t msg_len = strlen(received_msg);
            sent_bytes = send(remote_sock, received_msg, msg_len, 0);
            printf("Sent %d bytes of %lu bytes to \"%s\"\n", sent_bytes, msg_len, remote_addr);
        }
        
    }

    printf("Out of while loop.\n");
    clean_up(max_socket);
    return 0;
}