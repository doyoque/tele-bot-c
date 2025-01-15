#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
  int port;
  char *telegram_url;
  char *env;
} Config;

void parse_args(int argc, char *argv[], Config *config) {
  config->port = 0;
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

  printf("\n");
  printf("Env: %s\n", config.env ? config.env : "Not provided");
  printf("Port: %d\n", config.port);
  printf("Telegram URL: %s\n", config.telegram_url 
      ? config.telegram_url 
      : "Not provided"
  );

  return 0;
}
