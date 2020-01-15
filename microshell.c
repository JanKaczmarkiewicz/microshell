#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TRUE 1
#define SIZE 255

#define BUFFER_SIZE 1024

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
void inc();

char **readCommand(char *input);
void processCommand(char **command);

void comm_exit();
int comm_cd(char *path);
void comm_help();
void comm_history(struct Queue *history);
void comm_mv(char **command);

int hasAttr(char **command, char *attr);
int isAttr(char *comm);
int lastParamId(char **command);
int firstParamId(char **command);
int paramsCount(char **command);

char *concat2(char *source1, char *source2);
char *concat3(char *source1, char *source2, char *source3);

void moveFile(char *source, char*target);
void moveDirectory(char *source, char*target);
int fileType(char *path);

int main(void){
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

    if (strcmp(command[0], "cd") == 0) {
      if(comm_cd(command[1]) < 0) perror(command[1]);
      continue;
    }

    if (strcmp(command[0], "help") == 0) {
      comm_help(); continue;
    }

    if (strcmp(command[0], "history") == 0) {
      comm_history(history); continue;
    }

    if (strcmp(command[0], "mv") == 0) {
      comm_mv(command); continue;
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
  int quotesStart = 0;

  while (_in != NULL) {
    if (quotesStart == 1) {
      char *result = concat3(command[i - 1], " ", _in);
      command[i - 1] = result;
    } else {
      command[i] = _in;
      i++;
    }

    if ((int)_in[0] == 34) quotesStart = 1;
    else if ((int)_in[strlen(_in) - 1] == 34){
      quotesStart = 0;
      memmove(command[i - 1], command[i - 1] + 1, strlen(command[i - 1]));
      command[i - 1][strlen(command[i - 1]) - 1] = 0;
    }

    _in = strtok(NULL, " ");
  }
  command[i] = NULL;

  return command;
}

void processCommand(char **command) { // Przetwarzenie komendy z PATH
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
  printf("- history - wyświetla historię poleceń\n");
  printf("- obsługa argumentów w cudzysłowach\n");
}

void comm_history(struct Queue *history) {
  QUEUE_DISPLAY(history);
}

void comm_mv(char **command) {
  if (hasAttr(command, "-help")) {
    printf("help page\n");
    return;
  } else if (hasAttr(command, "-version")) {
    printf("version page\n");
    return;
  } else if(hasAttr(command, "t")) { // -t KATALOG ŹRÓDŁO...

  } else {
    int length = paramsCount(command);

    if (length == 2) {  // ŹRÓDŁO CEL
      int source = firstParamId(command);
      int target = lastParamId(command);

      if (fileType(command[source]) == 1) {
        moveFile(command[source], command[target]);
      } else if (fileType(command[source]) == 0) {
        moveDirectory(command[source], command[target]);
      }
    } else if(length > 2) { // ŹRÓDŁO... KATALOG
      int targetId = lastParamId(command);
      char *target = command[targetId];
      if (fileType(target) != 0) {
        printf("Last argument must be a directory.\n");
        return;
      }
      int type = fileType(command[firstParamId(command)]);

      for(int i = 1; i < targetId; i++) {
        if(isAttr(command[i])) continue;
        if(fileType(command[i]) != type) {
          printf("\"%s\" is not a %s.\n", command[i], type ? "file" : "directory");
          continue;
        }
        char *targetFile;
        if ((int)target[strlen(target) - 1] == 47) {
          targetFile = concat2(target, command[i]);
        } else {
          targetFile = concat3(target, "/", command[i]);
        }
        moveFile(command[i], targetFile);
      }
    }
  }
}

/* -------------------------------------------------------- */
/* -------------------- UTIL FUNCTIONS -------------------- */
/* -------------------------------------------------------- */

/* ------  Attributes and commands ------ */

int hasAttr(char **command, char *attr) {
  int i = 0;

  if (strlen(attr) > 1) {
    while (command[i] != NULL) {
      if(strlen(command[i]) < 2) continue;

      if (((int)command[i][0] == 45)
        && (strstr(command[i], attr) != NULL)) {
        return 1;
      }
      i++;
    }
  } else {
    while (command[i] != NULL) {
      if(strlen(command[i]) < 2) continue;

      if (((int)command[i][0] == 45)
        && ((int)command[i][1] != 45)
        && (strstr(command[i], attr) != NULL)) {
        return 1;
      }
      i++;
    }
  }


  return 0;
}

int isAttr(char *comm) {
  if ((int)comm[0] == 45) {
    return 1;
  }
  return 0;
}

int lastParamId(char **command) {
  int i = -1;

  while (command[i + 1] != NULL)
    i++;

  return i;
}

int firstParamId(char **command) {
  int i = 1;

  while (isAttr(command[i]))
    i++;

  return i;
}

int paramsCount(char **command) {
  int i = 1;
  int c = 0;

  while(command[i] != NULL) {
    if (!isAttr(command[i])) {
      c++;
    }
    i++;
  }

  return c;
}

/* ------ Strings ------ */
char *concat2(char *source1, char *source2) {
  char *result = malloc(strlen(source1) + strlen(source2) + 1);
  strcpy(result, source1);
  strcat(result, source2);
  return result;
}

char *concat3(char *source1, char *source2, char *source3) {
  char *result = malloc(strlen(source1) + strlen(source2) + strlen(source3) + 1);
  strcpy(result, source1);
  strcat(result, source2);
  strcat(result, source3);
  return result;
}

/* ------ Moving ------ */
void moveFile(char *source, char *target) {
  if (access(source, F_OK) == -1) {
    printf("Source file not found!\n");
    return;
  }

  char buffer[BUFFER_SIZE];
  int num;

  int fd_in = open(source, O_RDONLY);
  int fd_out = open(target, O_RDWR|O_CREAT, 0666);

  while ((num = read(fd_in, &buffer, BUFFER_SIZE)) > 0) {
    write(fd_out, &buffer, num);
  }

  close(fd_in);
  close(fd_out);

  remove(source);
}

void moveDirectory(char *source, char *target) {
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(source))) {
    // Empty directory
    return;
  }


  while ((entry = readdir(dir))) {
    if (entry->d_type == DT_DIR) {
      char path[BUFFER_SIZE];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      } else {
        char *comp_path = concat3(source, "/", entry->d_name);
        printf("  -%s\n", entry->d_name);
        moveDirectory(comp_path, target);
      }
    } else {
      printf("  %s\n", entry->d_name);
    }
  }
  closedir(dir);
}

int fileType(char *path) { // 0 -> directory, 1 -> file
  struct stat s;
  if (stat(path,&s) == 0) {
    if (s.st_mode & S_IFDIR) {
      return 0;
    } else if (s.st_mode & S_IFREG) {
      return 1;
    }
  }
}
