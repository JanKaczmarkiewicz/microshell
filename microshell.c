#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define SIZE 255

/* --------------------------------------------------------------- */
/* --------------------- QUEUE IMPLEMENTATION -------------------- */
/* ----------- Based on GeeksforGeeks implementation ------------- */
/* https://www.geeksforgeeks.org/queue-linked-list-implementation/ */
/* --------------------------------------------------------------- */

struct QNode {
  char *value;
  struct QNode *next;
};

struct Queue {
  struct QNode *front, *rear;
};

struct QNode *newNode(char *value) {
  struct QNode* nq = (struct QNode*)malloc(sizeof(struct QNode));
  nq->value = value;
  nq->next = NULL;
  return nq;
}

struct Queue *QUEUE() {
  struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
  q->front = q->rear = NULL;
  return q;
}

void QUEUE_ADD(struct Queue* q, char *value) {
  struct QNode* temp = newNode(value);

  if (q->rear == NULL) {
    q->front = q->rear = temp;
    return;
  }

  q->rear->next = temp;
  q->rear = temp;
}

void QUEUE_DISPLAY(struct Queue *q) {
  struct QNode *node = q->front;
  while (node != NULL) {
    printf("%s\n", node->value);
    node = node->next;
  }
}

/* -------------------------------------------------------- */
/* ------------------------- MAIN ------------------------- */
/* -------------------------------------------------------- */

void init();
char **readCommand(char *input);
void processCommand(char **command);
void inc();

void comm_exit();
int comm_cd(char *path);
void comm_help();
void comm_history(struct Queue *history);

int main(){
  char **command;
  char input[SIZE];

  struct Queue *history = QUEUE();

  init();

  while (TRUE) {
    inc();

    fgets(input, SIZE, stdin);
    strtok(input, "\n");

    char *coinput = (char *)malloc(sizeof(char *));
    strcpy(coinput, input);
    QUEUE_ADD(history, coinput);
    command = readCommand(input);

    if (strcmp(command[0], "exit") == 0)
      break;

    if(strcmp(command[0], "cd") == 0) {
      if(comm_cd(command[1]) < 0) perror(command[1]);
      continue;
    }

    if(strcmp(command[0], "help") == 0) {
      comm_help(); continue;
    }

    if(strcmp(command[0], "history") == 0) {
      comm_history(history); continue;
    }

    if (fork() == 0){
      processCommand(command);
    } else {
      wait(NULL);
    }
  }
  comm_exit();
  return 0;
}

void init() { // Wyświetlanie ekranu powitalnego
  printf("Welcome in Microshell!\n");
}

void inc() { // Wyświetlanie znaku zachęty
  char buffer[SIZE];
  printf("\033[1;32m");
  printf("[%s] $ ", getcwd(buffer, SIZE));
  printf("\033[0m");
}

char **readCommand(char *input) { // Odczyt komendy z ciągu znaków
  char **command = malloc(24 * sizeof(char *));

  char *_in = strtok(input, " ");
  int i = 0;

  while (_in != NULL) {
    command[i] = _in;
    i++;

    _in = strtok(NULL, " ");
  }
  command[i] = NULL;

  return command;
}

void processCommand(char **command) {
  if (execvp(command[0], command) < 0) {
    perror(command[0]);
    exit(1);
  }
  exit(0);
}

/* -------------------------------------------------------- */
/* ------------------ COMMANDS REGISTRY ------------------- */
/* -------------------------------------------------------- */

void comm_exit() {
  printf("Bye!\n");
}

int comm_cd(char *path) {
  return chdir(path);
}

void comm_help() {
  printf("MICROSHELL 0.2\nMarcin Czerniak, s452629\n");
  printf("FUNKCJONALNOŚCI:\n");
  printf("- wykonywanie programów opisanych w zmiennej PATH\n");
  printf("- cd - poruszanie się po katalogach\n");
  printf("- exit - kończy działanie programu\n");
  printf("- help - wyświetlanie informacji o autorze i funkcjonalnościach programu\n");
  printf("- history - wyświetla historię komend\n");
}

void comm_history(struct Queue *history) {
  QUEUE_DISPLAY(history);
}
