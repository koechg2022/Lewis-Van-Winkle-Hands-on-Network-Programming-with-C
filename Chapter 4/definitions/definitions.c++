#include "../headers/include.hpp"



bool is_caps(const char c) {
    return (c >= 'A' && c <= 'Z');
}


bool is_lower(const char c) {
    return (c >= 'a' && c <= 'z');
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


bool same_string(const char* first, const char* second, bool ignore_case, const char terminate) {
    int index;
    for (index = 0;
            !same_char(first[index], terminate) &&
            !same_char(second[index], terminate) &&
            same_char(first[index], second[index], ignore_case);
        index = index + 1);
    return same_char(first[index], second[index]) && same_char(first[index], terminate); // second check might not be necessary
}


char to_caps(const char c) {
    return (is_lower(c)) ? (c - ('a' - 'A')) : c;
}


char to_lower(const char c) {
    return (is_caps(c)) ? (c + ('a' - 'A')) : c;
}


bool is_initialized() {
    #if defined(crap_os)
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data)) {
            std::fprintf(stderr, "Failed to initialize.\n");
            is_init = false;
            return is_init;
        }
    #endif
    return is_init;
}


bool uninitialize() {
    #if defined(crap_os)
        WSACleanup();
    #endif
    return is_init;
}


int udp_client(const char* client, const char* port) {
    
    
    // Fucking windows
    bool was_init = is_init;
    if (!was_init) {
        if (!initialize()) {
            return 1;
        }
    }

    std::printf("Configuring the remote address...\n");
    struct addrinfo hints, *remote_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(client, port, &hints, &remote_address)) {
        std::fprintf(stderr, "Failed to get address information for remote machine. Error %d.\n", get_socket_errno());
        return 1;
    }

    
    char address_buff[buffer_size], service_buffer[buffer_size];

    if (getnameinfo(remote_address->ai_addr, remote_address->ai_addrlen, address_buff, buffer_size, service_buffer, buffer_size, NI_NUMERICHOST)) {
        std::fprintf(stderr, "Failed to retrieve address information. Error %d\n", get_socket_errno());
    }
    else {
        std::printf("Remote address is: %s : %s\n", address_buff, service_buffer);
    }

    std::printf("Creating the socket...\n");
    socket_type socket_peer = socket(remote_address->ai_family, remote_address->ai_socktype, remote_address->ai_protocol);

    if (!valid_socket(socket_peer)) {
        std::fprintf(stderr, "Failed to create the new connection socket. Error %d\n", get_socket_errno());
        freeaddrinfo(remote_address);
        return 1;
    }

    std::printf("Connecting the socket...\n");
    if (connect(socket_peer, remote_address->ai_addr, remote_address->ai_addrlen)) {
        std::fprintf(stderr, "Failed to connect the socket. Error %d\n", get_socket_errno());
        return 1;
    }

    freeaddrinfo(remote_address);

    std::printf("Connected.\nTo send data enter text, followed by enter.\n");

    while (true) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        #if defined(unix_os)
            FD_SET(STDIN_FILENO, &reads);
        #endif

        struct timeval timeout = {0, 100000};

        if (select(socket_peer + 1, &reads, 0, 0, &timeout) < 0) {
            std::fprintf(stderr, "Failed to select(). Error %d\n", get_socket_errno());
            return 1;
        }

        if (FD_ISSET(socket_peer, &reads)) {
            // There is data from the connected peer machine
            char read[4 * kilo_byte];
            int bytes_rec = recv(socket_peer, read, 4 * kilo_byte, 0);
            if (bytes_rec < 1) {
                std::printf("Connection closed dby peer.\n");
                break;
            }
            std::printf("Received (%d bytes) : %.*s\n", bytes_rec, bytes_rec, read);
        }

        if (
            #if defined(unix_os)
                FD_ISSET(STDIN_FILENO, &reads)
            #else
                _kbhit()
            #endif
        ) {
            // There is keyboard input
            char read[4 * kilo_byte];
            if (!fgets(read, 4 * kilo_byte, stdin)) {
                break;
            }
            std::printf("Sending : %s\n", read);
            int bytes_sent = send(socket_peer, read, std::strlen(read), 0);
            std::printf("Sent %d of %lu bytes.\n", bytes_sent, std::strlen(read));
        }

    }

    close_socket(socket_peer);
    if (!was_init) {
        uninitialize();
    }
    return 0;
}


int udp_recvfrom(const char *from_host, const char* port) {
    
    bool was_init = is_init;
    if (!was_init) {
        if (!initialize()) {
            return false;
        }
    }

    std::printf("Configuring local address...\n");
    struct addrinfo hints, *local_address;
    std::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &local_address)) {
        std::fprintf(stderr, "Failed to retrieve this machine's address information.\n");
        return 1;
    }

    std::printf("Creating a new socket...\n");
    socket_type connect_socket = socket(local_address->ai_family, local_address->ai_socktype, local_address->ai_protocol);

    if (!valid_socket(connect_socket)) {
        std::fprintf(stderr, "Failed to create a new valid socket.\n");
        return 1;
    }

    std::printf("Binding socket to local address.\n");
    if (bind(connect_socket, local_address->ai_addr, local_address->ai_addrlen)) {
        std::fprintf(stderr, "Failed to bind the socke to the local address.\n");
        freeaddrinfo(local_address);
        return 2;
    }

    freeaddrinfo(local_address);

    // The Socket has a local address assigned to it now.

    
    struct sockaddr_storage remote_address;
    socklen_t storage_len = sizeof(remote_address);
    char msg[kilo_byte];
    int bytes_rec = recvfrom(connect_socket, msg, kilo_byte, 0, (struct sockaddr*) &remote_address, &storage_len);

    std::printf("Received %d bytes.\n\n\"%.*s\"", bytes_rec, bytes_rec, msg);

    // See remote client's information
    char address_buffer[buffer_size], service_buffer[buffer_size];
    std::memset(address_buffer, 0, buffer_size);
    std::memset(service_buffer, 0, buffer_size);

    if (getnameinfo((struct sockaddr*) &remote_address, storage_len, address_buffer, buffer_size, service_buffer, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV)) {
        std::fprintf(stderr, "Error retrieving remote address and connection service values as a string.\n Error %d\n", get_socket_errno());
    }
    else {
        std::printf("\n\nRemote address is:\n");
        std::printf("\t%s : %s\n", address_buffer, service_buffer);
    }

    close_socket(connect_socket);
    
    if (!was_init) {
        uninitialize();
    }

    return 0;
}


int upd_sendto(const char* sendto_host, const char* port) {
    bool was_init = is_init;
    if (!was_init) {
        if (!initialize()) {
            std::fprintf(stderr, "Failed to initialize network for crap os\n");
            return 1;
        }
    }

    std::printf("Configuring remote address...\n");
    struct addrinfo hints, *this_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo("127.0.0.1", port, &hints, &this_address)) {
        std::fprintf(stderr, "Failed to retrieve loopback's address information.\n Error %d\n", get_socket_errno());
        return 1;
    }

    char address_buffer[buffer_size], service_buffer[buffer_size];
    if (getnameinfo(this_address->ai_addr, this_address->ai_addrlen, address_buffer, buffer_size, service_buffer, buffer_size, NI_NUMERICHOST | NI_NUMERICSERV)) {
        std::fprintf(stderr, "Failed to retrieve remote address information. Error %d\n", get_socket_errno());
    }
    else {
        std::printf("Remote address is %s : %s\n", address_buffer, service_buffer);
    }


    std::printf("Creating socket...\n");
    socket_type connect_socket = socket(this_address->ai_family, this_address->ai_socktype, this_address->ai_protocol);
    if (!valid_socket(connect_socket)) {
        std::fprintf(stderr, "Failed to create listening socket. Error %d\n", get_socket_errno());
        freeaddrinfo(this_address);
        return 1;
    }

    const char* msg = "Hello world from the upd_sendto function.";
    std::printf("Sending : %s\n", msg);

    int bytes_sent = sendto(connect_socket, msg, std::strlen(msg), 0, this_address->ai_addr, this_address->ai_addrlen);
    std::printf("Sent %d bytes of %lu bytes.\n", bytes_sent, std::strlen(msg));
    
    freeaddrinfo(this_address);
    close_socket(connect_socket);


    if (!was_init) {
        uninitialize();
    }

    return 0;
}


int upd_serve_toupper_simple(const char* client_host, const char* port) {

    bool was_init = is_init;
    if (!was_init) {
        if (initialize()) {
            std::fprintf(stderr, "Failed to initialize this crap operating system.\n");
            return 1;
        }
    }

    std::printf("Configuring local address.\n");
    struct addrinfo hints, *bind_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &bind_address)) {
        std::fprintf(stderr, "Failed to retrieve local machine's addres information. Socket error %d\n", get_socket_errno());
        return 1;
    }

    std::printf("Creating the connecting socket.\n");
    socket_type connection_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!valid_socket(connection_socket)) {
        std::fprintf(stderr, "Failed to create the socket. Error %d\n", get_socket_errno());
        freeaddrinfo(bind_address);
        return 1;
    }

    freeaddrinfo(bind_address);

    std::printf("Waiting for connections...\n");

    while (true) {
        struct sockaddr_storage client_address;
        socklen_t client_size = sizeof(client_address);

        char msg[kilo_byte];
        const int bytes_rec = recvfrom(connection_socket, msg, kilo_byte, 0, 
                    (struct sockaddr*) &client_address, &client_size);

        if (bytes_rec < 1) {
            std::fprintf(stderr, "Connection closed. Error %d\n", get_socket_errno());
            return 1;
        }

        // capitalize everything
        int index;
        for (index = 0; index < bytes_rec; index = index + 1) {
            msg[index] = to_caps(msg[index]);
        }
        sendto(connection_socket, msg, bytes_rec, 0, (struct sockaddr*) &client_address, client_size);
    }

    close_socket(connection_socket);

    if (!was_init) {
        uninitialize();
    }

    return 0;
}


int upd_serve_to_upper(const char* client_host, const char* port) {
    bool was_init = is_init;

    if (!was_init) {
        if (initialize()) {
            std::fprintf(stderr, "Failed to initialize network for crap os.\n");
            return 1;
        }
    }

    std::printf("Configuring local address...\n");
    struct addrinfo hints, *local_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(0, port, &hints, &local_address)) {
        std::fprintf(stderr, "Failed to retrieve local address information. Error %d\n", get_socket_errno());
        return 1;
    }

    std::printf("Creating the socket.\n");
    socket_type connect_socket = socket(local_address->ai_family, local_address->ai_socktype, local_address->ai_protocol);

    if (!valid_socket(connect_socket)) {
        std::fprintf(stderr, "Failed to create the socket. Error %d\n", get_socket_errno());
        freeaddrinfo(local_address);
        return 1;
    }

    std::printf("Binding the socket\n");
    if (bind(connect_socket, local_address->ai_addr, local_address->ai_addrlen)) {
        std::fprintf(stderr, "Failed to bind the socket to the local address. Error %d\n", get_socket_errno());
        freeaddrinfo(local_address);
        return 1;
    }

    freeaddrinfo(local_address);

    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(connect_socket, &master_set);
    socket_type max_socket = connect_socket;

    std::printf("Waiting for connections...\n");

    while (true) {
        fd_set reads_set;
        reads_set = master_set;

        if (select(max_socket + 1, &reads_set, 0, 0, 0) < 0 ) {
            std::fprintf(stderr, "Failed to select an active socket. Error %d\n", get_socket_errno());
            return 1;
        }

        if (FD_ISSET(connect_socket, &reads_set)) {
            struct sockaddr_storage client_address;
            socklen_t client_size = sizeof(client_address);

            char msg[kilo_byte];
            const int bytes_rec = recvfrom(connect_socket, msg, kilo_byte, 0, (struct sockaddr*) &client_address, &client_size);

            if (bytes_rec < 1) {
                std::fprintf(stderr, "Connection closed by remote machine. Error %d\n", get_socket_errno());
                return 1;
            }

            int index;
            for (index = 0; index < bytes_rec; index = index + 1) {
                msg[index] = to_caps(msg[index]);
            }

            sendto(connect_socket, msg, bytes_rec, 0, (struct sockaddr*) &client_address, client_size);

        }

    }

    close_socket(connect_socket);

    if (!was_init) {
        uninitialize();
    }

    return 0;
}