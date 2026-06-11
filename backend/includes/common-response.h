#ifndef COMMON_RESPONSE_H
#define COMMON_RESPONSE_H

static const char *const cors_headers =
    "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: "
    "GET, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, "
    "Accept\r\nAccess-Control-Expose-Headers: Content-Length, "
    "Content-Type" // Cache por 24 horas \r\nAccess-Control-Max-Age:
                   // 86400
    ;

int bad_request(int client_fd, unsigned long long buffer, const char *headers,
                const char *body);
int internal_server_error(int client_fd, unsigned long long buffer,
                          const char *headers, const char *body);

int not_found(int client_fd, unsigned long long buffer, const char *headers,
              const char *body);
int created(int client_fd, unsigned long long buffer, const char *headers,
            const char *body);

#endif // COMMON_RESPONSE_H
