#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFFER_SIZE 8192
#define DEFAULT_PORT 8080

typedef struct {
  int port;
  char *telegram_url;
  char *env;
} Config;

void send_response(
  int client_sock,
  int status_code,
  const char *content_type,
  const char *body
) {
  char header[BUFFER_SIZE];
  snprintf(
    header, 
    sizeof(header),
    "HTTP/1.1 %d OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "\r\n",
    status_code,
    content_type,
    strlen(body)
  );

  send(client_sock, header, strlen(header), 0);
  send(client_sock, body, strlen(body), 0);
}

void handle_client(int client_sock) {
  char buffer[BUFFER_SIZE];
  char method[16], path[256];
  ssize_t bytes_received;

  // receive the request
  bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received < 0) {
    perror("recv");
    close(client_sock);
    return;
  }

  buffer[bytes_received] = '\0';

  // parse the HTTP method and path
  if (sscanf(buffer, "%15s %255s", method, path) != 2) {
    send_response(client_sock, 400, "text/plain", "Bad Request");
    close(client_sock);
    return;
  }

  // log the request
  printf("received request: %s %s\n", method, path);

  // handle different paths
  if (strcmp(path, "/hello") == 0) {
    const char *json_body = "{\"message\": \"Hello, world!\"}\n";
    send_response(client_sock, 200, "application/json", json_body);
  } else if (strcmp(path, "/info") == 0) {
    const char *text_body = "This is a simple C HTTP server!\n";
    send_response(client_sock, 200, "text/plain", text_body);
  } else {
    const char *not_found_body = "{\"error\": \"Not found\"}\n";
    send_response(client_sock, 404, "application/json", not_found_body);
  }

  // close the connection
  close(client_sock);
}

void parse_args(int argc, char *argv[], Config *config) {
  config->port = DEFAULT_PORT;
  config->telegram_url = NULL;
  config->env = NULL;

  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--port=", 7) == 0) {
      config->port = atoi(argv[i] + 7);
    } else if (strncmp(argv[i], "--telegram_url=", 15) == 0) {
      config->telegram_url = argv[i] + 15;
    } else if (strncmp(argv[i], "--env=", 6) == 0) {
      config->env = argv[i] + 6;
    } else {
      printf("Unknown argument: %s\n", argv[i]);
    }
  }
}

int main(int argc, char *argv[]) {
  Config config;

  parse_args(argc, argv, &config);

  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  (void)server_addr;
  socklen_t client_len = sizeof(client_addr);

  // create the server socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("socket");
    return 1;
  }

  // configure server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(config.port);

  // bind the socket
  if (
    bind(
      server_sock,
      (struct sockaddr *)&server_addr, 
      sizeof(server_addr)
    ) < 0
  ) {
    perror("bind");
    close(server_sock);
    return 1;
  }

  // listen for incoming connections
  if (listen(server_sock, 10) < 0) {
    perror("listen");
    close(server_sock);
    return 1;
  }

  printf("\n");
  printf("Server listening on port %d\n", config.port);
  printf("Env: %s\n", config.env ? config.env : "Not provided");
  printf("Telegram URL: %s\n", config.telegram_url 
      ? config.telegram_url 
      : "Not provided"
  );

  while (1) {
    client_sock = accept(
      server_sock,
      (struct sockaddr *)&server_addr,
      &client_len
    );

    if (client_sock < 0) {
      perror("accept");
      continue;
    }

    handle_client(client_sock);
  }

  close(server_sock);
  return 0;
}
