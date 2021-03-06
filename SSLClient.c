    //============================================================================
// Name        : SSLClient.cpp
// Compiling   : g++ -c -o SSLClient.o SSLClient.cpp
//                 g++ -o SSLClient SSLClient.o -lssl -lcrypto
//                 g++ -o SSLClient SSLClient.o CFLAGS = -g -O3 -Wall -pedantic -mtune=native -I/usr/local/opt/openssl/include CXXFLAGS = -g -O3 -Wall -pedantic -mtune=native -I/usr/local/opt/openssl/include LDFLAGS = -L/usr/local/opt/openssl/lib
// I modified this so it would be a pure C compile instead of g++. gcc or cc will run the compile.
// cc -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -o SSLClient SSLClient.c -lssl -lcrypto
//============================================================================
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL *ssl;
int sock;

int RecvPacket()
{
    int len=100;
    int ret = 0;
    char buf[1000000];
    do {
        len=SSL_read(ssl, buf, 100);
        printf("LEN: %d", len);
        buf[len] = 0;
        printf("%s\n", buf);
    } while (len > 0);
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_READ)
            ret = 0;
        if (err == SSL_ERROR_WANT_WRITE)
            ret = 0;
        if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL)
            ret = -1;
    } else {
        ret = -1;
    }
    return ret;
}

int SendPacket(const char *buf)
{
    int len = SSL_write(ssl, buf, strlen(buf));
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        switch (err) {
            case SSL_ERROR_WANT_WRITE:
                return 0;
            case SSL_ERROR_WANT_READ:
                return 0;
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
            case SSL_ERROR_SSL:
            default:
                return -1;
        }
    } else {
        return -1;
    }
}


void log_ssl()
{
    int err;
    while ((err = ERR_get_error())) {
        char *str = ERR_error_string(err, 0);
        if (!str)
            return;
        printf("%s\n", str);
        fflush(stdout);
    }
}


int main(int argc, char *argv[])
{
    // set domian name to "login.salesforce.com"
    struct hostent* hostname = gethostbyname("login.salesforce.com");
    struct in_addr addr;
    memcpy(&addr, hostname->h_addr_list[0], sizeof(struct in_addr)); 
    char *ip = inet_ntoa(addr);
    printf("IP: %s\n", ip);

    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (!s) {
        printf("Error creating socket.\n");
        return -1;
    }
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof(sa));
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip); // address of login.salesforce.com
    sa.sin_port        = htons (443); 
    socklen_t socklen = sizeof(sa);
    if (connect(s, (struct sockaddr *)&sa, socklen)) {
        printf("Error connecting to server.\n");
        return -1;
    }
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = TLSv1_2_client_method();
    SSL_CTX *ctx = SSL_CTX_new (meth);
    ssl = SSL_new (ctx);
    if (!ssl) {
        printf("Error creating SSL.\n");
        log_ssl();
        return -1;
    }
    sock = SSL_get_fd(ssl);
    SSL_set_fd(ssl, s);
    int err = SSL_connect(ssl);
    if (err <= 0) {
        printf("Error creating SSL connection.  err=%x\n", err);
        log_ssl();
        fflush(stdout);
        return -1;
    }
    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

    char *request = "GET https://login.salesforce.com/ HTTP/1.1\r\nHost: login.salesforce.com \r\n\r\n";
    SendPacket(request);
    RecvPacket();
    return 0;
}