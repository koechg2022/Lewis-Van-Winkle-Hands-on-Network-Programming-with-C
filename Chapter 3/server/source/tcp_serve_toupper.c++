

#include "tcp_serve_chat"

int main(int len, char** args) {
    if (len > 2) {
        fprintf(stderr, "Usage : %s <port | None> (Either enter something or nothing for the port)\n", args[0]);
        return ILLEG_ARGS;
    }
    return (len == 2) ? run_server(args[1]) : run_server();
}


bool is_caps(const char c) {
    return (c >='A' && c <= 'Z');
}

bool is_lower(const char c) {
    return (c >= 'a' && c <= 'z');
}

bool same_char(const char a, const char b, bool ignore_case) {
    return (ignore_case) ? (to_caps(a) == to_caps(b)) : a == b;
}

bool same_string(const char* first, const char* second, bool ignore_case) {
    unsigned long index;
    for (index = 0; first[index] != '\0' && second[index] != '\0' && !same_char(first[index], second[index], ignore_case); index = index + 1);
    return first[index] == '\0' && second[index] == '\0';
}

char to_caps(const char c) {
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}

char to_lower(const char c) {
    return ((is_caps(c))) ? (c + ('a' - 'A')) : c;
}

void capitalize(char* the_string) {
    unsigned long index;
    for (index = 0; the_string[index] != '\0'; index = index + 1) {
        the_string[index] = to_caps(the_string[index]);
    }
}

void lowerize(char* the_string) {
    int index;
    for (index = 0; the_string[index] != '\0'; index = index + 1) {
        the_string[index] = to_lower(the_string[index]);
    }
}

int initialize() {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            return MACHINE_INIT_FAIL;
        }
    #endif
    return 0;
}

int clean_up(socket_type max_socket) {
    int e_code;
    socklen_t e_code_size = sizeof(e_code);
    socket_type this_socket;
    for (this_socket = 0; this_socket <= max_socket; this_socket = this_socket + 1) {
        if (is_std_socket(this_socket)) {
            continue;
        }

        if (get_socket_option(this_socket, e_code, e_code_size)) {
            continue;
        }

        close_socket(this_socket);
    }
    #if defined(crap_os)
        WSACleanup();
    #endif
    return 0;
}

int run_server(char* port) {
    if (initialize()) {
        fprintf(stderr, "Yo... you're trying to run a server on Windows... Freakin' Windows needs network initialization but this machine failed to initialize! Error %d.\n", get_socket_errno());
        return MACHINE_INIT_FAIL;
    }

    struct addrinfo hints, *server_machine;
    char this_addr[buffer_size], this_serv[buffer_size], message[kilo_byte];
    socket_type listen_sock, max_socket, this_sock, client_sock;
    struct sockaddr_storage new_client;
    socklen_t client_size;
    int socket_count;
    printf("Configuring local address...\n");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &server_machine)) {
        fprintf(stderr, "Failed to retrieve this machine's address information. Not ideal. Error %d...\n", get_socket_errno());
        return MACHINE_ADDR_RET_FAIL;
    }

    // Retrieved this machine's information.
    if (getnameinfo(server_machine->ai_addr, server_machine->ai_addrlen, this_addr, buffer_size, this_serv, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV)) {
        fprintf(stderr, "Failed to retrieve this machine's machine address as a char* IP address. Error %d", get_socket_errno());
        return MACHINE_ADDR_RET_FAIL;
    }

    printf("Creating socket...\n");
    listen_sock = socket(server_machine->ai_family, server_machine->ai_socktype, server_machine->ai_protocol);
    if (!valid_socket(listen_sock)) {
        fprintf(stderr, "Failed to create the listening socket for this machine (%s)\n", this_addr);
        freeaddrinfo(server_machine);
        return SOCKET_CREAT_FAIL;
    }

    printf("Binding socket to local address...\n");
    if (bind(listen_sock, server_machine->ai_addr, server_machine->ai_addrlen)) {
        fprintf(stderr, "Failed to bind the listening socket to a local address. Error %d\n", get_socket_errno());
        freeaddrinfo(server_machine);
        return BIND_SOCKET_FAIL;
    }
    freeaddrinfo(server_machine);
    printf("Listening socket has been bound to local address.\n\nTo Connect to this this server, use this address and service:\n");
    printf("\tIP Address : %s\n\tService/Port : %s\n", this_addr, this_serv);

    if (listen(listen_sock, listen_lim)) {
        fprintf(stderr, "Failure to listen on the bound socket. Error %d\n", get_socket_errno());
        close_socket(listen_sock);
        return LISTEN_SOCKET_FAIL;
    }

    fd_set master, reads;

    FD_ZERO(&master);
    FD_SET(listen_sock, &master);
    max_socket = listen_sock;
    socket_count = 1;
    printf("Waiting for connections...\n");
    bool exit_prog = false;
    while (true) {
        // reads = master;
        FD_COPY(&master, &reads);
        // printf("reads is now :\n");
        // for (this_sock = 1; this_sock <= max_socket; this_sock = this_sock + 1) {
        //     printf("this_sock\t:\t%d\n", this_sock);
        // }
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "Failed to correctly select a socket. Something went wrong. Error %d\n", get_socket_errno());
            clean_up(max_socket);
            return SEL_SOCKET_FAIL;
        }

        // printf("Reached.\n");

        for (this_sock = 0; this_sock <= max_socket; this_sock = this_sock + 1) {
            
            if (FD_ISSET(this_sock, &reads)) {

                if (is_std_socket(this_sock)) {
                    printf("Ignoring std socket.\n");
                    continue;
                }
                
                else if (this_sock == listen_sock) {
                    client_size = sizeof(new_client);
                    client_sock = accept(this_sock, (struct sockaddr*) &new_client, &client_size);
                    if (!valid_socket(client_sock)) {
                        fprintf(stderr, "Failed to accept a new incomming connection. Could not create new socket. Error %d\n", get_socket_errno());
                        clean_up(max_socket);
                        return NEW_CONNECT_FAIL;
                    }

                    FD_SET(client_sock, &master);

                    if (client_sock > max_socket) {
                        max_socket = client_sock;
                    }

                    if (getnameinfo((struct sockaddr*) &new_client, client_size, this_addr, buffer_size, this_serv, buffer_size, NI_NUMERICHOST)) {
                        fprintf(stderr, "Failed to convert new connection information into a readable IP address. Error %d\n", get_socket_errno());
                        clean_up(max_socket);
                        return NEW_CONNECT_FAIL;
                    }

                    printf("New connection from:\n\t%s\n\t\t%s\n", this_addr, this_serv);
                    socket_count = socket_count + 1;

                }

                
                else {
                    printf("In else branch\n");
                    int bytes_received = recv(this_sock, message, kilo_byte, 0);
                    if (bytes_received < 1) {
                        FD_CLR(this_sock, &master);
                        close_socket(this_sock);
                        if (socket_count - 1 == 1) {
                            memset(message, 0, kilo_byte);
                            printf("There are no more connections to this socket. Close the server? (enter 'yes' to close, anything else otherwise): ");
                            fgets(message, kilo_byte, stdin);
                            if (same_string(message, (char *) "yes") || same_char(message[0], 'y')) {
                                exit_prog = true;
                                break;
                            }
                        }
                        continue;
                    }
                    printf("Bytes received \"%.*s\"\n", bytes_received, message);
                    message[bytes_received] = '\0';
                    printf("Received:\n\"%s\"\n", message);
                    capitalize(message);
                    printf("About to send:\n\"%s\"\n", message);
                    send(this_sock, message, bytes_received, 0);
                }

            }


        }

        if (exit_prog) {
            break;
        }

    }

    clean_up(max_socket);
    printf("Finished\n");
    return 0;
}


socket_type max_socket(socket_type first, socket_type second) {
    return (first >= second) ? first : second;
}