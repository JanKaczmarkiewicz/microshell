#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
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
  struct QNode *head;
  struct QNode *tail;
};

struct Queue *QUEUE() {
  struct Queue *q = (struct Queue *)malloc(BUFFER_SIZE);
  q->head = NULL;
  q->tail = NULL;
  return q;
}

void QUEUE_ADD(struct Queue* q, char *value) {
  struct QNode *node = (struct QNode *)malloc(BUFFER_SIZE);
  node->value = value;
  node->next = NULL;
  struct QNode* temp = node;

  if (q->tail == NULL) {
    q->head = temp;
    q->tail = temp;
    return;
  }

  q->tail->next = temp;
  q->tail = temp;
}

void QUEUE_DISPLAY(struct Queue *q) {
  struct QNode *node = q->head;
  while (node != NULL) {
    printf("%s\n", node->value);
    node = node->next;
  }
}

char *QUEUE_LAST(struct Queue *q) {
  return q->tail->value;
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
char *withoutSlash(char *str);

void moveFile(char *source, char*target, int verbose);
void moveDirectory(char *source, char *target, int verbose);
void moveDir(char *source, char *target, int verbose);
void renameDirectory(char *source, char *target, int verbose);
int createDirectory(char *path);
int fileType(char *path);

int main(void){
  char **command;
  char *input = (char *)malloc(SIZE);

  struct Queue *history = QUEUE();

  char *lastDirectory = (char *)malloc(SIZE);

  init();

  while (TRUE) {
    inc();

    fgets(input, SIZE, stdin);
    strtok(input, "\n");

    char *coinput = (char *)malloc(BUFFER_SIZE);

    if (strcmp(input, "!!") == 0) { // TODO: strstr()
      input = QUEUE_LAST(history);
    }

    strcpy(coinput, input);
    QUEUE_ADD(history, coinput);
    command = readCommand(input);

    if (strcmp(command[0], "exit") == 0)
      break;

    if (strcmp(command[0], "cd") == 0) {
      if (paramsCount(command) < 1) {
        printf("Usage: cd [path]\n");
        continue;
      }

      if (strcmp(command[1], "-") == 0) {
        if (lastDirectory != NULL && comm_cd(lastDirectory) < 0) {
          perror(lastDirectory);
        }
        continue;
      }

      lastDirectory = getcwd(lastDirectory, SIZE);

      if (comm_cd(command[1]) < 0) {
        perror(command[1]);
      }
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

    if (strcmp(command[0], "nano") == 0 || strcmp(command[0], "vim") == 0) {
      printf("\n");
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
  printf("------------------------------------\n");
  printf("- Microshell, by Marcin Czerniak. - \n");
  printf("------------------------------------\n");
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
    } else if(i > 0 && ((int)command[i - 1][strlen(command[i - 1]) - 1] == 92)) {
      command[i - 1][strlen(command[i - 1]) - 1] = 0;
      char *result = concat3(command[i - 1], " ", _in);
      command[i - 1] = result;
    } else {
      command[i] = _in;

      if (command[i][0] == 34 && command[i][strlen(command[i]) - 1] == 34) {
        memmove(command[i], command[i] + 1, strlen(command[i]));
        command[i][strlen(command[i]) - 1] = 0;
      }

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
  if ((int)path[0] == 126) {
    path = memmove(path, path + 1, strlen(path));
    path = concat3(getenv("HOME"), "/", path);
  }
  return chdir(path);
}

void comm_help() {
  printf("\nMICROSHELL 1.0r1\nMarcin Czerniak, s452629\n\n");
  printf("FUNKCJONALNOŚCI:\n");
  printf("- wykonywanie programów opisanych w zmiennej PATH\n");
  printf("- cd - poruszanie się po katalogach (obsługa: '~', '-')\n");
  printf("- exit - kończy działanie programu\n");
  printf("- help - wyświetlanie informacji o autorze i funkcjonalnościach programu\n");
  printf("- history - wyświetla historię poleceń\n");
  printf("- obsługa argumentów w cudzysłowach\n");
  printf("- obsługa '\\' przy wieloczłonowych argumentach\n");
  printf("- !! - wykonanie ostatnio wprowadzonego polecenia\n");
  printf("- własna implementacja polecenia: 'mv'\n");
}

void comm_history(struct Queue *history) {
  QUEUE_DISPLAY(history);
}

void comm_mv(char **command) {
  if (hasAttr(command, "-help") || command[1] == NULL) { // HELP
    printf("Usage: mv [OPTION]... SOURCE DEST\n");
    printf("\tor: mv [OPTION]... SOURCE... DIRECTORY\n");
    printf("\tor: mv [OPTION]... -t DIRECTORY SOURCE...\n");
    printf("\nOPTIONS:\n");
    printf("-v, --verbose\texplain what is being done\n");
  } else if (hasAttr(command, "-version")) { // VERSION
    printf("'mv' implementation v1.0\n");
  } else if (paramsCount(command) < 2) { // NOT ENOUGH ARGUMENTS
    printf("Not enough arguments. Help: 'mv --help'\n");
  } else if(hasAttr(command, "t")) { // -t DEST SOURCE...
    int length = paramsCount(command);

    if (paramsCount(command) < 3) {
      printf("Not enough arguments. Help: 'mv --help'\n");
    } else {
      int verbose = 0;
      if (hasAttr(command, "v") || hasAttr(command, "-verbose")) {
        verbose = 1;
      }
      int targetId = firstParamId(command);
      char *target = command[targetId];

      if (fileType(target) != 0) {
        printf("First argument must be a directory.\n");
      } else {
        for(int i = 1; i <= lastParamId(command); i++) {
          if(isAttr(command[i])) continue;

          if(fileType(command[i]) == 1) { // ... FILE DIR
            char *targetFile;
            if ((int)target[strlen(target) - 1] == 47) {
              targetFile = concat2(target, command[i]);
            } else {
              targetFile = concat3(target, "/", command[i]);
            }
            moveFile(command[i], targetFile, verbose);
          } else { // ... DIR DIR
            char *res = concat3(withoutSlash(target), "/", command[i]);
            renameDirectory(command[i], res, verbose);
          }
        }
      }
    }
  } else {
    int verbose = 0;
    if (hasAttr(command, "v") || hasAttr(command, "-verbose")) {
      verbose = 1;
    }
    int length = paramsCount(command);

    if (length == 2) {  // SOURCE DEST
      int source = firstParamId(command);
      int target = lastParamId(command);

      if (fileType(command[source]) == 1) { // FILE ...
        if (fileType(command[target]) == 0) { // FILE DIR
          if ((int)command[target][strlen(command[target]) - 1] != 47) {
            char *newTarget = concat2(command[target], "/");
            command[target] = newTarget;
          }
          command[target] = concat2(command[target], basename(command[source]));
          moveFile(command[source], command[target], verbose);
        } else { // FILE FILE
          moveFile(command[source], command[target], verbose);
        }
      } else if (fileType(command[source]) == 0) { // DIR DIR
        renameDirectory(command[source], command[target], verbose);
      }
    } else if(length > 2) { // SOURCE... DEST
      int targetId = lastParamId(command);
      char *target = command[targetId];
      if (fileType(target) != 0) {
        printf("Last argument must be a directory.\n");
        return;
      }

      for(int i = 1; i < targetId; i++) {
        if(isAttr(command[i])) continue;

        if(fileType(command[i]) == 1) { // ... FILE DIR
          char *targetFile;
          if ((int)target[strlen(target) - 1] == 47) {
            targetFile = concat2(target, command[i]);
          } else {
            targetFile = concat3(target, "/", command[i]);
          }
          moveFile(command[i], targetFile, verbose);
        } else { // ... DIR DIR
          char *res = concat3(withoutSlash(target), "/", command[i]);
          renameDirectory(command[i], res, verbose);
        }
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
        && (int)command[i][1] == 45
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
  if (strlen(comm) > 1 && (int)comm[0] == 45) {
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

char *withoutSlash(char *str) {
  if ((int)str[strlen(str) - 1] == 47) {
    str[strlen(str) - 1] = 0;
  }
  return str;
}

/* ------ Moving ------ */
void moveFile(char *source, char *target, int verbose) {
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

  if (verbose)
    printf("%s -> %s\n", source, target);

  close(fd_in);
  close(fd_out);

  remove(source);
}

void moveDirectory(char *source, char *target, int verbose) {
  DIR *dir;
  struct dirent *entry;

  createDirectory(target);

  if ((int)target[strlen(target) - 1] == 47) {
    createDirectory(concat2(target, source));
  } else {
    createDirectory(concat3(target, "/", source));
  }

  if (!(dir = opendir(source))) {
    return;
  }

  while ((entry = readdir(dir))) {
    if (entry->d_type == DT_DIR) {
      char path[BUFFER_SIZE];
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      } else {
        char *comp_path = concat3(source, "/", entry->d_name);
        char *comp_target = concat3(target, "/", entry->d_name);
        if (verbose)
          printf("%s -> %s\n", comp_path, comp_target);
        moveDirectory(comp_path, comp_target, verbose);
        rmdir(comp_path);
      }
    } else {
      char *comp_path = concat3(source, "/", entry->d_name);
      char *comp_target = concat3(target, "/", entry->d_name);
      moveFile(comp_path, comp_target, verbose);
    }
  }
  closedir(dir);
}

void moveDir(char *source, char *path, int verbose) {
  moveDirectory(source, path, verbose);
  rmdir(source);
}

void renameDirectory(char *source, char *path, int verbose) {
  moveDir(source, path, verbose);
  rmdir(concat3(withoutSlash(path), "/", source));
}

/* ------ Directories ------ */

int createDirectory(char *path) {
  struct stat fstat = {0};

  if (stat(path, &fstat) < 0) {
    mkdir(path, 0777);
    return 1;
  }
  return 0;
}

int fileType(char *path) { // 0 -> directory, 1 -> file, -1 -> not exist
  struct stat s;
  if (stat(path, &s) == 0) {
    if (s.st_mode & S_IFDIR) {
      return 0;
    } else if (s.st_mode & S_IFREG) {
      return 1;
    }
  }
  return -1;
}

