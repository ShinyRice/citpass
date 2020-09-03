#include <sodium.h> /* Encryption */
#include <stdio.h> /* fputs, fgets... */
#include <stdlib.h> /* File I/O */
#include <string.h> /* String manipulation */
#include <sys/stat.h> /* Creating folders */
#include <sys/types.h>
#include <termios.h> /* Telling the terminal to not show input */
#include <time.h> /* Initializing seed for random generation */
#include <unistd.h>

#define RANDSTR_LEN 50
#define TITLE_LEN 100

/* Functions */
/* Setting the path to the directory where passwords are stored, done through an environment variable */
void setting_dirpath(char* homepath, char dirpath[500]) {
  char* storelocation = getenv("CITPASS_DIR");
  /* If storelocation is NULL, that is, it's an empty string, we jump to the else case. If it's not empty
   * we use it as the password folder location */
  if (storelocation) {
    strncpy(dirpath, storelocation, 400);
  }
  else {
    strncpy(dirpath, homepath, 400);
    strncat(dirpath, "/.local/share/citpass", 400);
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
      puts("citpass requires a command. Possible commands are:");
      puts("init - Create the folder where passwords will be stored, located at $HOME/.local/share/citpass");
      puts("add - Add a password along with associated information to said file");
      puts("ls - List all entries for which there is a password");
      puts("rm - Remove a password along with associated information from the file");
      puts("get - Retrieve a password");
      break;
    case 1:
      puts("Invalid command, please provide a valid one.");
      puts("Possible commands are:");
      puts("init - Create the folder where passwords will be stored, located at $HOME/.local/share/citpass");
      puts("add - Add a password along with associated information to said file");
      puts("ls - List all entries for which there is a password");
      puts("rm - Remove a password along with associated information from the file");
      puts("get - Retrieve a password");
      break;
    case 2:
      puts("This command does not need arguments.");
      break;
    case 3:
      puts("This command requires the title of an entry in order to proceed.");
      break;
    case 4:
      puts("Too many arguments have been passed.");
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

void parse_index_file(char* buffer, size_t size) {
  /* Let's first figure out how many lines we have in the file, by counting the amount of newline characters, */
  int lines = 0;
  for (int i = 0; i < size - 1; i++) {
    if (buffer[i] == '\n') {
      lines++;
    }
  }
  /* Since we don't know how many passwords a user has stored in the folder,
   * it's necessary to dynamically allocate memory for the array that'll hold
   * the title strings. It's a 2D character array, so a 1D string array.
   * We already know how long titles can get, as defined in this constant.
   * A pointer to a pointer is more or less a 2D array. */
  char **titles = malloc(lines*sizeof(char));
  /* Error handling */
  if (titles == NULL) {
    puts("Failed to allocate needed memory for reading index file. Aborting.");
    free(titles);
    free(buffer);
    exit(EXIT_FAILURE);
  }
  for(int i = 0; i < lines; i++) {
    titles[i] = malloc(TITLE_LEN*sizeof(char));
    /* Error handling */
    if (titles[i] == NULL) {
      puts("Failed to allocate needed memory for reading index file. Aborting.");
      free(titles);
      free(buffer);
      exit(EXIT_FAILURE);
    }
  }
  int n = 0;
  int p = 0;
  while (n < size - 1) {
    if (buffer[n] == ',') {
      while (buffer[n] != '\n' && n < size - 1) {
        titles[p][n] = buffer[n];
        n++;
      }
      p++;
    }
    n++;
  }
  for(int i = 0; i < lines; i++) {
    fputs(titles[i], stdout);
  }
  free(titles);
}

/* Initializing store */
void initalization(char* dirpath, char* indexpath) {
  if (sodium_init() < 0) {
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
    exit(EXIT_FAILURE);
  }
  /* Here, we check if the directory exists, */
  if (access(dirpath, F_OK) != -1) {
    fputs("The folder at ", stdout);
    fputs(dirpath, stdout);
    fputs(" exists.\n", stdout);

    if (access(indexpath, F_OK) != -1) {
      /* And here, we check if the index file within exists as well, */
      fputs("The index file at ", stdout);
      fputs(indexpath, stdout);
      fputs(" exists as well. No action necessary.\n", stdout);
    }
    else {
      /* This is the case where the folder exists, but the file doesn't. The file is promptly created. */
      puts("The index file doesn't exist. Creating it.");

      FILE *indexcheck = fopen(indexpath, "w");
      fclose(indexcheck);
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dirpath, stdout);
    fputs(" doesn't exist. Creating it.\n", stdout);
    if (mkdir(dirpath, 0700) == -1) {
      puts("Creating folder failed. Aborting.");
    }
    else {
      puts("Creating index file within folder as well.");

      FILE *indexcheck = fopen(indexpath, "w");
      fclose(indexcheck);
    }
  }
}

/* Adding a password to the folder, and adding the random filename to the index */
void add_password(char* dirpath, char* indexpath, char* filepath) {
  if (sodium_init() < 0) {
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
    exit(EXIT_FAILURE);
  }
  char randstr[RANDSTR_LEN];
  char title[TITLE_LEN];
  char password[100];
  char username[100];
  char url[200];
  char notes[1000];
  char indexentry[150];

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dirpath, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* We initialize the seed for generating random strings, */
      srand(time(NULL));
      /* then generate such a string, */
      rand_string(randstr, 50);
      /* and append it to the folder path, */
      strncat(filepath, randstr, 500);

      /* File is opened, and subsequently the user fills the file with the password and relevant metadata */
      FILE *fileadd = fopen(filepath, "a");
      fputs("Title: ", stdout);
      fgets(title, 100, stdin);
      fputs("Title: ", fileadd);
      fputs(title, fileadd);

      fputs("Password: ", stdout);

      /* These 5 lines down here are required for preventing password input from being shown */
      struct termios oldt;
      tcgetattr(STDIN_FILENO, &oldt);
      struct termios newt = oldt;
      newt.c_lflag &= ~ECHO;
      tcsetattr(STDIN_FILENO, TCSANOW, &newt);

      /* Here's the actual input */
      fgets(password, 100, stdin);

      /* Here the terminal is brought back to how it was */
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

      fputs("Password: ", fileadd);
      fputs(password, fileadd);
      fputs("\n", stdout);

      fputs("Username: ", stdout);
      fgets(username, 100, stdin);
      fputs("Username: ", fileadd);
      fputs(username, fileadd);

      fputs("URL: ", stdout);
      fgets(url, 200, stdin);
      fputs("URL: ", fileadd);
      fputs(url, fileadd);

      fputs("Notes: ", stdout);
      fgets(notes, 1000, stdin);
      fputs("Notes: ", fileadd);
      fputs(notes, fileadd);

      fclose(fileadd);

      /* File's closed, and thus now we append the title of the entry and corresponding randomized filename to the end of the index file */
      FILE *indexadd = fopen(indexpath, "a");

      strncpy(indexentry, randstr, 140);
      strncat(indexentry, ",", 140);
      strncat(indexentry, title, 140);

      fputs(indexentry, indexadd);

      fclose(indexadd);
   }
   else {
     puts("The index file doesn't exist. Please run \"citpass init\" to create it.");
   }
 }
 else {
   fputs("The folder at ", stdout);
   fputs(dirpath, stdout);
   fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
 }
}

void list_passwords(char* indexpath) {
  if (sodium_init() < 0) {
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
    exit(EXIT_FAILURE);
  }
  /* Index file decryption */
  FILE *indexfile = fopen(indexpath, "r");
  /* Error handling */
  if (indexfile == NULL) {
    puts("Failed to open index file. Aborting.");
    fclose(indexfile);
    exit(EXIT_FAILURE);
  }

  /* It's necessary to find out the size of the file. */

  /* File descriptor being set, */
  int fd = fileno(indexfile);
  /* Error handling */
  if (fd == -1) {
    puts("Could not read index file. Aborting.");
    fclose(indexfile);
    exit(EXIT_FAILURE);
  }

  struct stat buf;
  /* With fstat() we get file attributes and put them in buf. buf.st_size is
   * the size of the file in bytes. */
  fstat(fd, &buf);
  off_t filesize = buf.st_size;

  /* Error handling */
  if ((fstat(fd, &buf) != 0) || (!S_ISREG(buf.st_mode))) {
    puts("Could not read index file. Aborting.");
    fclose(indexfile);
    exit(EXIT_FAILURE);
  }

  /* I'll set a large upper limit for the file, 1 MB. */
  if (filesize > 1000000) {
    puts("Index file is larger than 1 MB. Aborting.");
    fclose(indexfile);
    exit(EXIT_FAILURE);
  }

  /* File's characters will be stored at this pointer, we're setting buffer up to point
   * at the allocated memory with the file we're going to read's filesize */
  char* buffer = (char*)malloc(filesize);

  /* Error handling */
  if (buffer == NULL) {
    puts("Failed to allocate needed memory for reading index file. Aborting.");
    fclose(indexfile);
    exit(EXIT_FAILURE);
  }

  /* And so we finally write the file to memory. getc reads one character at a time,
   * every time it is called it reads the next character */
  char c;
  int n = 0;
  while ((c = getc(indexfile)) != EOF) {
    buffer[n] = c;
    n++;
  }

  fclose(indexfile);

  /* Now, we get how many characters are stored in the buffer, */
  size_t size = (int) sizeof(buffer)/sizeof(char);
  printf("%lu", size);

  /* As well as the amount of lines in the buffer, done by counting
   * newline characters and adding one to that, */

  parse_index_file(buffer, size);

  free(buffer);

  /* Index file encryption */
}

void rm_password(char* indexpath) {
  if (sodium_init() < 0) {
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
    exit(EXIT_FAILURE);
  }
  FILE *indexfile = fopen(indexpath, "rw");

  /* Index file decryption */

  /* Print out list of entries */

  /* Index file encryption */

  /* User selects entry */

  /* Entry is deleted */

  fclose(indexfile);
}

void get_password(char* indexpath) {
  if (sodium_init() < 0) {
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
    exit(EXIT_FAILURE);
  }
  FILE *indexfile = fopen(indexpath, "r");

  /* Index file decryption */

  /* Print out list of entries */

  /* Index file encryption */

  fclose(indexfile);

  /* User selects entry */

  /* Password file decryption */

  /* Password is printed to stdout/piped to clipboard manager */

  /* Password file encryption */
}

int main(int argc, char *argv[]) {
  char homepath[100];
  char dirpath[500];
  strncpy(homepath, getenv("HOME"), 100);
  setting_dirpath(homepath, dirpath);

  strncpy(dirpath, homepath, 400);
  strncat(dirpath, "/.local/share/citpass", 400);

  char indexpath[500];
  strncpy(indexpath, dirpath, 400);
  strncat(indexpath, "/index", 400);

  char filepath[500];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(filepath, dirpath, 400);
  strncat(filepath, "/", 400);

  /* Now, it's necessary to parse the command passed to the program, so */
  int init = strncmp(argv[1], "init", 5);
  int add = strncmp(argv[1], "add", 5);
  int ls = parse_ls(argv[1]);
  int rm = strncmp(argv[1], "rm", 5);
  int get = strncmp(argv[1], "get", 5);

  /* First, it's necessary to know how many commands have been passed. */
  switch (argc) {
  case 1:
    /* This first case below executes when just the binary's name has been invoked, */
    show_command_information(0);
    break;
  case 2:
    /* Here's the case when a command's been passed to the program, */
    if (init == 0) {
      initalization(dirpath, indexpath);
    }
    else if (add == 0) {
      add_password(dirpath, indexpath, filepath);
    }
    else if (ls == 0) {
      list_passwords(indexpath);
    }
    else if (rm == 0) {
      show_command_information(3);
    }
    else if (get == 0) {
      show_command_information(3);
    }
    else {
      show_command_information(1);
    }
    break;
  case 3:
    /* And this is when a command's been passed as well as one argument, */
    if (init == 0) {
      show_command_information(2);
    }
    else if (add == 0) {
      show_command_information(2);
    }
    else if (ls == 0){
      show_command_information(2);
    }
    else if (rm == 0) {
      rm_password(indexpath);
    }
    else if (get == 0) {
      get_password(indexpath);
    }
    else {
      show_command_information(1);
    }
    break;
  default:
    show_command_information(4);
  }
  return 0;
}
