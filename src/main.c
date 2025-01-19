#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFFER_SIZE 8192
#define DEFAULT_PORT 8080

int server_sock = -1;

typedef struct {
  const char *key;
  const char *value;
} Json;

typedef struct {
  int port;
  char *telegram_url;
  char *env;
} Config;

// construct the JSON from key-value pairs
char *construct_json_response(Json fields[], size_t field_count) {
  size_t buffer_size = BUFFER_SIZE;
  char *json = malloc(buffer_size);
  if (!json) {
    perror("malloc");
    exit(1);
  }

  strcpy(json, "{");
  for (size_t i = 0; i < field_count; i++) {
    size_t remaining_space = buffer_size - strlen(json) - 1;
    char field[BUFFER_SIZE];
    snprintf(
      field,
      sizeof(field),
      "\"%s\": \"%s\"%s",
      fields[i].key,
      fields[i].value,
      (i == field_count - 1) ? "" : ", "
    );

    if (strlen(field) >= remaining_space) {
      buffer_size *= 2;
      json = realloc(json, buffer_size);
      if (!json) {
        perror("realloc");
        exit(1);
      }
    }

    strcat(json, field);
  }
  strcat(json, "}");

  return json;
}

// construct the HTTP response string dynamically
char *construct_http_response(
  int status_code,
  const char *content_type,
  const char *body
) {
  const char *status_text;
  switch(status_code) {
    case 200: status_text = "OK"; break;
    case 400: status_text = "Bad Request"; break;
    case 404: status_text = "Not Found"; break;
    default: status_text = "Internal Server Error"; break;
  }

  size_t body_length = strlen(body);
  size_t response_size = BUFFER_SIZE;
  char *response = malloc(response_size);
  if (!response) {
    perror("malloc");
    exit(1);
  }

  snprintf(
    response,
    response_size,
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "\r\n"
    "%s",
    status_code,
    status_text,
    content_type,
    body_length,
    body
  );

  return response;
}

void send_response(
  int client_sock,
  int status_code,
  const char *content_type,
  const char *body
) {
  char *response = construct_http_response(status_code, content_type, body);
  send(client_sock, response, strlen(response), 0);
  free(response);
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
    Json fields[] = {{"message", "Hello, world!"}};
    char *json_body = construct_json_response(fields, 1);
    send_response(client_sock, 200, "application/json", json_body);
    free(json_body);
  } else if (strcmp(path, "/doyoque") == 0) {
    Json fields[] = {{"name", "Hello, world!"}, {"gender", "male"}};
    char *json_body = construct_json_response(fields, 2);
    send_response(client_sock, 200, "application/json", json_body);
    free(json_body);
  } else if (strcmp(path, "/info") == 0) {
    const char *text_body = "This is a simple C HTTP server!\n";
    send_response(client_sock, 200, "text/plain", text_body);
  } else {
    Json fields[] = {{"error", "Not Found"}};
    char *json_body = construct_json_response(fields, 1);
    send_response(client_sock, 404, "application/json", json_body);
    free(json_body);
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

void handle_signal(int signal) {
  if (signal == SIGINT) {
    printf("\nShutting down the server gracefully...\n");
    if (server_sock >= 0) {
      close(server_sock);
      printf("Server socket closed.\n");
    }
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  Config config;

  parse_args(argc, argv, &config);

  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  (void)server_addr;
  socklen_t client_len = sizeof(client_addr);

  signal(SIGINT, handle_signal);

  // create the server socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
