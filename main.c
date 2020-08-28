#include <stdio.h> /* printf, scanf, fputs, fgets... */
#include <stdlib.h> /* File I/O */
#include <string.h> /* String manipulation */
#include <sys/stat.h> /* Creating folders */
#include <termios.h>
#include <unistd.h>
#include <sodium.h> /* Encryption */

/* Functions */
/* Setting the path where passwords are store, done through an environment variable */
void setting_folderpath(char* homepath, char folderpath[500]) {
  char* storelocation = getenv("CITPASS_STORE");

  if (strncmp(storelocation, "", 400) == 0) {
    strncpy(folderpath, homepath, 400);
    strncat(folderpath, "/.local/share/citpass", 400);
  }
  else {
    strncpy(folderpath, storelocation, 400);
  }
}

/* Parsing possible commands for listing passwords */
int parse_ls(char* list) {
  int result;
  if (strncmp(list, "ls", 5) == 0 || strncmp(list, "list", 5) == 0 || strncmp(list, "show", 5) == 0) {
    result = 0;
  }
  else {
    result = 1;
  }
  return result;
}

void show_command_information(int situation) {
  switch (situation) {
    case 0:
      printf("citpass requires a command. Possible commands are:\n");
      printf("init - Create the folder where passwords will be stored, located at $HOME/.local/share/citpass\n");
      printf("add - Add a password along with associated information to said file\n");
      printf("ls - List all entries for which there is a password\n");
      printf("rm - Remove a password along with associated information from the file\n");
      printf("get - Retrieve a password\n");
      break;
    case 1:
      printf("Invalid command, please provide a valid one.\n");
      printf("Possible commands are:\n");
      printf("init - Create the folder where passwords will be stored, located at $HOME/.local/share/citpass\n");
      printf("add - Add a password along with associated information to said file\n");
      printf("ls - List all entries for which there is a password\n");
      printf("rm - Remove a password along with associated information from the file\n");
      printf("get - Retrieve a password\n");
      break;
    case 2:
      printf("This command does not need arguments.");
      break;
    case 3:
      printf("This command requires the title of an entry in order to proceed.");
  }
}

/* Generating a random string */
static char* rand_string(char* str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  if (size) {
    --size;
    for (size_t n = 0; n < size; n++) {
      int key = rand() % (int) (sizeof charset - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

/* Initializing store */
void init(char* folderpath, char* indexpath) {
/* Here, we check if the folder exists, */
  if (access(folderpath, F_OK) != -1) {
    printf("The folder at %s exists.\n", folderpath);
    if (access(indexpath, F_OK) != -1) {
      /* And here, we check if the index file within exists as well, */
      printf("The index file at %s exists as well. No action necessary.\n", indexpath);
    }
    else {
      /* This is the case where the folder exists, but the file doesn't. The file is promptly created. */
      printf("The index file doesn't exist. Creating it.\n");

      FILE *indexcheck;
      indexcheck = fopen(indexpath, "w");
      fclose(indexcheck);
    }
  }
  else {
    printf("The folder at %s doesn't exist. Creating it.\n", folderpath);
    if (mkdir(folderpath, 0700) == -1) {
      printf("Creating folder failed. Aborting.\n");
    }
    else {
      printf("Creating index file within folder as well.\n");

      FILE *indexcheck;
      indexcheck = fopen(indexpath, "w");
      fclose(indexcheck);
    }
  }
}

/* Adding a password to the folder, and adding the random filename to the index */
void add_password(char* folderpath, char* indexpath, char* filepath) {
  char randstr[100];
  char title[100];
  char password[100];
  char username[100];
  char url[200];
  char notes[1000];
  /* Before doing anything, we need to know whether or not the application folder in ~/.local/share
  * and the file within exist. */

  /* Here, we check if the folder exists, */
  if (access(folderpath, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* We generate a random string, */
      rand_string(randstr, 100);
      /* Append it to the folder path, */
      strncat(filepath, randstr, 500);

      /* File is opened, and subsequently the user fills the file with the password and relevant metadata */

      FILE *fileadd;
      fileadd = fopen(filepath, "a");
      printf("Title: ");
      fgets(title, 100, stdin);
      fprintf(fileadd, "Title: %s", title);

      printf("Password: ");

      /* These 5 lines here are required for hiding password input from being outputted */
      struct termios oldt;
      tcgetattr(STDIN_FILENO, &oldt);
      struct termios newt = oldt;
      newt.c_lflag &= ~ECHO;
      tcsetattr(STDIN_FILENO, TCSANOW, &newt);

      /* Here's the actual input */
      fgets(password, 100, stdin);

      /* Here the terminal is brought back to how it was */
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

      fprintf(fileadd, "Password: %s", password);

      printf("Username: ");
      fgets(username, 100, stdin);
      fprintf(fileadd, "Username: %s", username);

      printf("URL: ");
      fgets(url, 200, stdin);
      fprintf(fileadd, "URL: %s", url);

      printf("Notes: ");
      fgets(notes, 1000, stdin);
      fprintf(fileadd, "Notes: %s", notes);

      fclose(fileadd);

      /* File's closed, and thus now we append the title of the entry and corresponding randomized filename to the end of the index file */
      FILE *indexadd;
      indexadd = fopen(indexpath, "a");

      fprintf(indexadd, "%s,", title);
      fprintf(indexadd, "%s", randstr);

      fclose(indexadd);
   }
   else {
     printf("The index file doesn't exist. Please run \"citpass init\" to create it.\n");
   }
 }
 else {
   printf("The folder at %s doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", folderpath);
 }
}

void list_passwords(char* indexpath) {
      FILE *indexfile;
      indexfile = fopen(indexpath, "r");

      /* File decryption */

      /* Appending an empty entry to the end of the database file */

      /* User now fills the entry with information */

      fclose(indexfile);

      /* File encryption */
}

void rm_password(char* indexpath) {
      FILE *filerm;
      filerm = fopen(indexpath, "rw");

      /* File decryption */

      /* Print out list of entries */

      /* User selects entry */

      /* Entry is deleted */

      fclose(filerm);

      /* File encryption */
}

void get_password(char* indexpath) {
      FILE *indexfile;
      indexfile = fopen(indexpath, "r");

      /* File decryption */

      /* Print out list of entries */

      /* User selects entry */

      /* Password is printed to stdout */

      fclose(indexfile);

      /* File encryption */
}

int main(int argc, char *argv[]) {
  char* homepath = getenv("HOME");
  char folderpath[500];
  setting_folderpath(homepath, folderpath);

  char indexpath[500];
  strncpy(indexpath, folderpath, 400);
  strncat(indexpath, "/index", 400); /* In the future, this'll be set by the user, through a configuration file or an argument */

  char filepath[500];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(filepath, folderpath, 400);
  strncat(filepath, "/", 400);

  /* Now, it's necessary to parse the command passed to the program, so */
  int arginit = strncmp(argv[1], "init", 5);
  int argadd = strncmp(argv[1], "add", 5);
  int argls = parse_ls(argv[1]);
  int argrm = strncmp(argv[1], "rm", 5);
  int argget = strncmp(argv[1], "get", 5);

  /* First, it's necessary to know how many commands have been passed. This first case below
  * executes when just the binary's name has been invoked, */
  if (argc == 1) {
    show_command_information(0);
  }
  /* Here's the case when a command's been passed to the program, */
  else if (argc == 2) {
    if (arginit == 0) {
      /* Initialization
      * Before doing anything, we need to know whether or not the password store in ~/.local/share/citpass
      * and the index file within exists. If both exist, then nothing is done. If the folder exists, but the file doesn't,
      * only the file is created. If the folder doesn't exist, then both the folder and the file within are created.
      */
      init(folderpath, indexpath);
    }
    else if (argadd == 0) {
      add_password(folderpath, indexpath, filepath);
    }
    else if (argls == 0) {
      list_passwords(indexpath);
    }
    else if (argrm == 0) {
      show_command_information(3);
    }
    else if (argget == 0) {
      show_command_information(3);
    }
    else {
      show_command_information(1);
    }
  }
  else if (argc > 2) {
    if (arginit == 0) {
      show_command_information(2);
    }
    else if (argadd == 0) {
      show_command_information(2);
    }
    else if (argls == 0){
      show_command_information(2);
    }
    else if (argrm == 0) {
      rm_password(indexpath);
    }
    else if (argget == 0) {
      get_password(indexpath);
    }
    else {
      show_command_information(1);
    }
  }
  return 0;
}
