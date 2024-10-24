#include "headers.h"



#define TIMEOUT 5.0



void parse_url(char* url, char** hostname, char** port, char** path);



void send_request(SSL* s, char* hostname, char* port, char* path);



socket_type connect_to_host(char* hostname, char* port);



int main(int len, char** args) {

    if (len < 2) {
        std::fprintf(stderr, "Usage: %s URL\n", *args);
        return 1;
    }

    #if defined(_WIN32)
        WSADATA d;

        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            std::fprintf(stderr, "Failed to initialize.\n");
            return 1;
        }

    #endif

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX* context = SSL_CTX_new(TLS_client_method());

    if (not context) {
        std::fprintf(stderr, "SSL_CTX_new() failed.\n");
        return 1;
    }

    char* hostname, *port, *path;
    parse_url(*(args + 1), &hostname, &port, &path);


    socket_type server = connect_to_host(hostname, port);

    SSL* ssl = SSL_new(context);
    if (not ssl) {
        std::fprintf(stderr, "SSL_new() failed.\n");
        close_socket(server);
        std::exit(1);
    }

    if (not SSL_set_tlsext_host_name(ssl, hostname)) {
        std::fprintf(stderr, "SSL_set_tlsext_host_name() failed.\n");
        ERR_print_errors_fp(stderr);
        close_socket(server);
        std::exit(1);
    }

    SSL_set_fd(ssl, server);
    if (SSL_connect(ssl) == -1) {
        std::fprintf(stderr, "SSL_connect() failed.\n");
        close_socket(server);
        ERR_print_errors_fp(stderr);
        std::exit(1);
    }

    std::printf("SSL/TLS using %s\n", SSL_get_cipher(ssl));

    X509* certificate = SSL_get_peer_certificate(ssl);
    if (not certificate) {
        std::fprintf(stderr, "SSL_get_peer_certificate() failed.\n");
        close_socket(server);
        std::exit(1);
    }

    char* temp;
    if ((temp = X509_NAME_oneline(X509_get_subject_name(certificate), 0, 0))) {
        std::printf("Subject: %s\n", temp);
        OPENSSL_free(temp);
    }


    if ((temp = X509_NAME_oneline(X509_get_issuer_name(certificate), 0, 0))) {
        std::printf("Issuer: %s\n", temp);
        OPENSSL_free(temp);
    }

    X509_free(certificate);

    send_request(ssl, hostname, port, path);

    const std::clock_t start_time = std::clock();

    char response[response_size], *q, *body = 0;
    char* end = response + response_size, *p = response;

    typedef enum {length, chunked, connection} msg_type;
    int remaining = 0;
    msg_type encoding = length;

    while (true) {

        if ((((double) (std::clock() - start_time)) / CLOCKS_PER_SEC) > TIMEOUT) {
            std::fprintf(stderr, "Timeout after %.2f seconds\n", TIMEOUT);
            std::exit(1);
        }

        if (p == end) {
            std::fprintf(stderr, "Out of buffer space.\n");
            std::exit(1);
        }

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server, &reads);

        struct timeval timeout = (struct timeval) {0, 200};
        
        if (select(server + 1, &reads, 0, 0, &timeout) < 0) {
            std::fprintf(stderr, "select() failed. Error %d\n", get_socket_error());
            std::exit(1);
        }

        if (FD_ISSET(server, &reads)) {

            int bytes_received = SSL_read(ssl, p, end - p);
            if (bytes_received < 1) {
                if (encoding == connection and body) {
                    std::printf("%.*s", (int) (end - body), body);
                }

                std::printf("\nConnection closed by peer.\n");
                break;
            }



            p = p + bytes_received;
            *p = 0;

            if (not body and (body = std::strstr(response, "\r\n\r\n"))) {
                *body = 0;
                *body = *body + 4;

                std::printf("Received Headers:\n%s\n", response);

                q = std::strstr(response, "\nContent-Length: ");
                if (q) {
                    encoding = length;
                    q = std::strchr(q, ' ');
                    q = q + 1;
                    remaining = std::strtol(q, 0, 10);
                }

                else {
                    q = std::strstr(response, "\nTransfer-Encoding: chunked");
                    if (q) {
                        encoding = chunked;
                        remaining = 0;
                    }
                    else {
                        encoding = connection;
                    }
                }
                std::printf("\nReceived Body:\n");
            }

            if (body) {
                if (encoding == length) {
                    if (p - body >= remaining) {
                        std::printf("%.*s", remaining, body);
                        break;
                    }
                }

                else if (encoding == chunked) {
                    do {

                        if (remaining == 0) {
                            std::strtol(body, 0, 16);
                            if ((q = std::strstr(body, "\r\n"))) {
                                if (not remaining) goto finish;
                                body = q + 2;
                            }
                            else {
                                break;
                            }
                        }
                        
                        if (remaining and p - body >= remaining) {
                            std::printf("%.*s", remaining, body);
                            body = body + remaining + 2;
                            remaining = 0;
                        }

                    } while (not remaining);
                }
            }

        }

    }
    finish:

    std::printf("\nClosing socket...\n");
    SSL_shutdown(ssl);
    close_socket(server);
    SSL_free(ssl);
    SSL_CTX_free(context);

    #if defined(_WIN32)
        WSACleanup();
    #endif

    std::printf("Finished.\n");
    std::exit(0);
    return 0;
}




void parse_url(char* url, char** hostname, char** port, char** path) {
    std::printf("URL: \"%s\"\n", url);

    char* pointer;

    pointer = std::strstr(url, "://");
    char* protocol = 0;

    if (pointer) {
        protocol = url;
        *pointer = 0;
        pointer = pointer + 3;
    }

    else {
        pointer = url;
    }

    if (protocol) {
        if (std::strcmp(protocol, "https")) {
            std::fprintf(stderr, "Unknown protocol '%s'. Only 'https' is supported.\n", protocol);
            std::exit(1);
        }
    }

    *hostname = pointer;
    while (*pointer and *pointer NOT ':' and *pointer NOT '/' and *pointer NOT '#') pointer++;

    *port = (char*) "443";

    if (*pointer == ':') {
        *pointer++ = 0;
        *port = pointer;
    }

    while (*pointer and *pointer NOT '/' and *pointer NOT '#') ++pointer;

    *path = pointer;
    if (*pointer == '/') {
        *path = pointer + 1;
    }
    *pointer = 0;

    while (*pointer and *pointer NOT '#') ++pointer;

    if (*pointer == '#') *pointer = 0;

    std::printf("hostname : %s\n", *hostname);
    std::printf("port: %s\n", *port);
    std::printf("path: %s\n", *path);

}


void send_request(SSL* s, char* hostname, char* port, char* path) {
    char buffer[two_kilo_bytes];
    std::memset(buffer, 0, two_kilo_bytes);

    std::snprintf(buffer, two_kilo_bytes, "GET /%s HTTP/1.1\r\n", path);
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "Connection: close\r\n");
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "User-Agent: honpwc https_get 1.0\r\n");
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "\r\n");

    SSL_write(s, buffer, std::strlen(buffer));
    std::printf("Sent Headers:\n%s", buffer);
}


socket_type connect_to_host(char* hostname, char* port) {
    std::printf("Configuring remote address...\n");
    struct addrinfo hints, *remote_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &remote_address)) {
        std::fprintf(stderr, "getaddrinfo() failed. Error %d\n", get_socket_error());
        std::exit(1);
    }

    std::printf("Remote address is: ");
    char address[buffer_size], service[buffer_size];
    getnameinfo(remote_address->ai_addr, remote_address->ai_addrlen, address, buffer_size, service, buffer_size, NI_NUMERICHOST);
    std::printf("%s %s\n", address, service);

    std::printf("Creating socket...\n");
    socket_type the_answer;
    the_answer = socket(remote_address->ai_family, remote_address->ai_socktype, remote_address->ai_protocol);
    
    if (not valid_socket(the_answer)) {
        std::fprintf(stderr, "socket() failed. Error %d\n", get_socket_error());
        std::exit(1);
    }

    std::printf("Connecting...\n");
    if (connect(the_answer, remote_address->ai_addr, remote_address->ai_addrlen)) {
        std::fprintf(stderr, "connect() failed. Error %d\n", get_socket_error());
        std::exit(1);
    }

    freeaddrinfo(remote_address);

    std::printf("Connected.\n\n");
    return the_answer;
}