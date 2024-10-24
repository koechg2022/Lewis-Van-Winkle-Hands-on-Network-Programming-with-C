#include "headers.h"


bool verify_cert = false;


int main(int len, char** args) {

    if (len < 3) {
        std::fprintf(stderr, "Usage: %s hostname port\n", *args);
        return 1;
    }

    char* hostname = *(args + 1), *port = *(args + 2);

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


    std::printf("Configuring remote address...\n");
    struct addrinfo hints, *remote_address;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hostname, port, &hints, &remote_address)) {
        std::fprintf(stderr, "getaddrinfo() failed. (%d)\n", get_socket_error());
        std::exit(1);
    }

    std::printf("Remote address is: ");
    char address_buffer[buffer_size], service_buffer[buffer_size];
    getnameinfo(remote_address->ai_addr, remote_address->ai_addrlen, hostname, buffer_size, port, buffer_size, NI_NUMERICHOST);

    std::printf("%s %s\n", address_buffer, service_buffer);

    std::printf("Creating socket...\n");
    socket_type server = socket(remote_address->ai_family, remote_address->ai_socktype, remote_address->ai_protocol);

    if (not valid_socket(server)) {
        std::fprintf(stderr, "socket() failed. (%d)\n", get_socket_error());
        std::exit(1);
    }

    std::printf("Connecting...\n");
    if (connect(server, remote_address->ai_addr, remote_address->ai_addrlen)) {
        std::fprintf(stderr, "connect() failed. (%d)\n", get_socket_error());
        std::exit(1);
    }

    freeaddrinfo(remote_address);
    std::printf("Connected.\n\n");

    SSL* ssl = SSL_new(context);
    if (not ssl) {
        std::fprintf(stderr, "SSL_new() failed.\n");
        std::exit(1);
    }

    if (not SSL_set_tlsext_host_name(ssl, hostname)) {
        std::fprintf(stderr, "SSL_set_txsext_host_name() failed.\n");
        ERR_print_errors_fp(stderr);
        std::exit(1);
    }

    SSL_set_fd(ssl, server);
    if (SSL_connect(ssl) == -1) {
        std::fprintf(stderr, "SSL_connect() failed.\n");
        ERR_print_errors_fp(stderr);
        exit(1);
    }


    std::printf("SSL/TLS using %s\n", SSL_get_cipher(ssl));

    X509* certificate = SSL_get_peer_certificate(ssl);
    if (not certificate) {
        std::fprintf(stderr, "SSL_get_peer_certificate() failed.\n");
        std::exit(1);
    }

    char* temp;
    if ((temp = X509_NAME_oneline(X509_get_subject_name(certificate), 0, 0))) {
        std::printf("subject: %s\n", temp);
        OPENSSL_free(temp);
    }

    if ((temp = X509_NAME_oneline(X509_get_issuer_name(certificate), 0, 0))) {
        std::printf("issuer: %s\n", temp);
        OPENSSL_free(temp);
    }

    X509_free(certificate);

    char buffer[two_kilo_bytes];
    std::memset(buffer, 0, two_kilo_bytes);

    std::snprintf(buffer, two_kilo_bytes, "GET / HTTP/1.1\r\n");
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "Connection: close\r\n");
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer), "User-Agent: https_simple\r\n");
    std::snprintf(buffer + std::strlen(buffer), two_kilo_bytes - std::strlen(buffer),"\r\n");


    SSL_write(ssl, buffer, std::strlen(buffer));
    std::printf("Sent Headers:\n%s", buffer);
    std::memset(buffer, 0, sizeof(buffer));
    while (true) {

        int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_received < 1) {
            std::printf("\nConnection closed by peer.\n");
            break;
        }

        std::printf("Received (%d bytes): '%.*s'\n", bytes_received, bytes_received, buffer);
    }

    std::printf("\nClosing socket...\n");
    SSL_shutdown(ssl);
    close_socket(server);
    SSL_free(ssl);
    SSL_CTX_free(context);

    #if defined(_WIN32)
        WSACleanup();
    #endif
    

    std::printf("Finished.\n");
    return 0;
}