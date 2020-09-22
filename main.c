/* Forgive the excess of obvious comments in this source. They somewhat ease subsequent readings of the code
 * that I make, to consolidate what I learnt already. */

#define _POSIX_C_SOURCE 200809L
/* C standard library, part of glibc */
#include <stdio.h> /* fputs, fgets... */
#include <stdlib.h> /* File I/O */
#include <string.h> /* String manipulation */
#include <time.h> /* Initializing seed for random generation */
/* C POSIX library, part of glibc */
#include <sys/stat.h> /* Creating folders */
#include <sys/types.h>
#include <termios.h> /* Telling the terminal to not show input */
#include <unistd.h>
/* Libsodium */
#include <sodium.h> /* Encryption */

#define RANDSTR_LEN 50
#define TITLE_LEN 100
#define PASS_LEN 200

/* In order to avoid getting a nice fat warning, a quick workaround is to declare fileno() explicitly in the source
 * I wrote, instead of having that done in the corresponding header file...
 * https://stackoverflow.com/questions/46213840/get-rid-of-warning-implicit-declaration-of-function-fileno-in-flex */
int fileno(FILE *stream);

/* Functions */
void show_command_information(const int sit) {
  switch (sit) {
    case 0:
      fputs("citpass requires a command. Possible commands are:\n", stdout);
      fputs("init - Create the folder where passwords and index will be stored, located at $HOME/.local/share/citpass\n", stdout);
      fputs("add - Create a password entry\n", stdout);
      fputs("ls - List all password entries\n", stdout);
      fputs("rm - Remove a password entry\n", stdout);
      fputs("get - Retrieve a password\n", stdout);
      break;
    case 1:
      fputs("Invalid command, please provide a valid one.\n", stdout);
      fputs("Possible commands are:\n", stdout);
      fputs("init - Create the folder where passwords and index will be stored, located at $HOME/.local/share/citpass\n", stdout);
      fputs("add - Create a password entry\n", stdout);
      fputs("ls - List all password entries\n", stdout);
      fputs("rm - Remove a password entry\n", stdout);
      fputs("get - Retrieve a password\n", stdout);
      break;
    case 2:
      fputs("This command does not need arguments.\n", stdout);
      break;
    case 3:
      fputs("This command requires the title of an entry in order to proceed.\n", stdout);
      break;
    case 4:
      fputs("Too many arguments have been passed.\n", stdout);
  }
}

/* Setting the path to the directory where passwords are stored, done through an environment variable */
void setting_dir_path(const char* home_path, char* dir_path) {
  char* store_location = getenv("CITPASS_DIR");
  /* If storelocation is null, it'll be 0. It's a pointer pointing to nothing, in other words, the environment
   * variable hasn't been set and is just an empty string, and so we jump to the else case, since 0 is a boolean false.
   * If it's not empty we use it as the password folder location */
  if (store_location) {
    strncpy(dir_path, store_location, 200);
  }
  else {
    strncpy(dir_path, home_path, 200);
    strncat(dir_path, "/.local/share/citpass", 200);
  }
}

/* Fetching password from stdin, not letting it show up on the terminal */
void password_input(char* password) {
  /* These 5 lines down here are required for preventing password input from being shown */
  struct termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  struct termios newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  /* Here's the actual input */
  fgets(password, PASS_LEN, stdin);

  /* And the terminal is brought back to how it was */
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

/* Generating a random string */
static char* rand_junk_str(char* str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  if (size) {
    --size;
    for (size_t n = 0; n < size; n++) {
      /* I won't use rand() for crypto, so don't panic. */
      int key = rand() % (int) (sizeof charset - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

/* fp == file pointer */
off_t get_file_size(const char* path) {
  /* It's necessary to find out the size of the file.
  * In order to do that, first we find out the file descriptor, fd, */
  FILE* fp = fopen(path, "r");
  int fd = fileno(fp);
  /* Error handling */
  if (fd == -1) {
    fputs("Could not read index file. Aborting.", stdout);
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  struct stat buf;
  /* With fstat() we get file attributes and put them in buf. buf.st_size is
   * the size of the file in bytes. */
  int stat = fstat(fd, &buf);
  /* Error handling */
  if ((stat != 0) || (!S_ISREG(buf.st_mode))) {
    fputs("Could not read index file. Aborting.", stdout);
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  off_t file_size = buf.st_size;
  /* I'll set a large upper limit for the file, 1 MB. */
  if (file_size > 1000000) {
    fputs("Index file is larger than 1 MB. Aborting.", stdout);
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  fclose(fp);
  return file_size;
}

size_t read_index_file(FILE* index_fp, char* buffer) {
  /* We write the file to memory. getc() reads one character at a time,
   * every time it is called it reads the next character */
  char c;
  unsigned int n = 0;

  while ((c = getc(index_fp)) != EOF) {
    buffer[n] = c;
    n++;
  }
  size_t buf_len = n + 1;
  return buf_len;
}

/* The way this function parses the index file puts the entire second column of the CSV index file
 * into titles[], which means that "Titles" will invariably be the first string in that array.
 * We won't ignore that when parsing, but we will ignore it when printing titles[], simply by starting
 * from titles[1] and not titles[0] */
void parse_index_file(char** titles, const unsigned int lines, const char* buffer, const size_t buf_len) {
  unsigned int n = 0;
  unsigned int p = 0;
  unsigned int m;

  /* In this loop, we're just walking through each character of the 1D array that is the file buffer,
   * until we reach the last element of the array */
  while (n < buf_len) {
    /* If we find a comma, that means we've reached a title. */
    if (buffer[n] == ',') {
       /* We advance by one char, and find the first char of the title, */
      n++;
      /* And set a variable which will be the index of the string where the title'll be stored */
      m = 0;
      /* Now, we walk char by char through the buffer again, storing characters in the string at titles[p] until we find
       * a newline char, and until the end of the file, */
      while (buffer[n] != '\n' && n < buf_len) {
        titles[p][m] = buffer[n];
        m++;
        n++;
      }
      /* Since this is done char by char, instead of treating everything with string manipulation
       * functions, we need to add the termination character at the end of the string, */
      m++;
      titles[p][m] = '\0';
      /* And if we're not at the end of the buffer, we jump to the next title string */
      if (n < buf_len) {
        p++;
      }
    }
    n++;
  }
}

static int encrypt(const char* dest_file_path, const char* message, const size_t message_len) {
  unsigned char ciphertext[message_len + crypto_secretbox_MACBYTES];
  char mast_pass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mast_pass);
  fputs("\n", stdout);
  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[crypto_secretbox_KEYBYTES];

  randombytes_buf(salt, sizeof(salt));
  if (crypto_pwhash(key, sizeof(key), mast_pass, strlen(mast_pass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    fputs("Ran out of memory while deriving key from master password. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  crypto_secretbox_easy(ciphertext, (unsigned char*)message, message_len, salt, key);

  FILE* dest_fp = fopen(dest_file_path, "wb");
  if (! dest_fp) {
    fputs("Failed to open file. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  fwrite(ciphertext, 1, message_len + crypto_secretbox_MACBYTES, dest_fp);
  fclose(dest_fp);
  return 0;
}

static int decrypt(const char* src_file_path, const char* message, const size_t message_len) {
  char mast_pass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mast_pass);

  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[crypto_secretbox_KEYBYTES];

  randombytes_buf(salt, sizeof(salt));
  fputs("\n", stdout);
  if (crypto_pwhash(key, sizeof(key), mast_pass, strlen(mast_pass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    fputs("Ran out of memory while deriving key from master password. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  FILE* dest_fp = fopen(src_file_path, "rb");
  if (! dest_fp) {
    fputs("Failed to open file. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  /* It's necessary to know how much memory to allocate to get the file onto memory.
   * In order to do that, we need the file size; but most crypto algos usually add
   * a fixed number of bytes to the file. I'm going to take advantage of this fact. */
  unsigned char ciphertext[message_len + crypto_secretbox_MACBYTES];
  fread(ciphertext, sizeof(char), message_len + crypto_secretbox_MACBYTES, dest_fp);
  if (crypto_secretbox_open_easy((unsigned char*)message, ciphertext, message_len + crypto_secretbox_MACBYTES, salt, key) != 0) {
    fputs("File contents have been forged or corrupted. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  fclose(dest_fp);
  return 0;
}


void initialize() {
  char home_path[100];
  strncpy(home_path, getenv("HOME"), 100);
  char dir_path[200];
  setting_dir_path(home_path, dir_path);

  char index_path[300];
  strncpy(index_path, dir_path, 280);
  strncat(index_path, "/index", 280);

  if (access(dir_path, F_OK) != -1) {
    fputs("The folder at ", stdout);
    fputs(dir_path, stdout);
    fputs(" exists.\n", stdout);
    if (access(index_path, F_OK) != -1) {
      fputs("The index file at ", stdout);
      fputs(index_path, stdout);
      fputs(" exists as well. No action necessary.\n", stdout);
    }
    else {
      fputs("Creating index file within folder.\n", stdout);
      if (encrypt(index_path, "Filename,Title", strlen("Filename,Title")) != 0) {
        fputs("Failed to encrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dir_path, stdout);
    fputs(" doesn't exist. Creating it.\n", stdout);
    if (mkdir(dir_path, 0700) == -1) {
      fputs("Creating folder failed. Aborting.\n", stdout);
      exit(EXIT_FAILURE);
    }
    else {
      fputs("Creating index file within folder.\n", stdout);
      if (encrypt(index_path, "Filename,Title", strlen("Filename,Title")) != 0) {
        fputs("Failed to encrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
    }
  }
}

/* Adding a password to the folder, and adding the random filename to the index */
void add_password() {
  char home_path[100];
  strncpy(home_path, getenv("HOME"), 100);
  char dir_path[200];
  setting_dir_path(home_path, dir_path);

  char index_path[300];
  strncpy(index_path, dir_path, 280);
  strcat(index_path, "/index");

  char file_path[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(file_path, dir_path, 280);
  strcat(file_path, "/");

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(index_path, F_OK) != -1) {
      char rand_str[RANDSTR_LEN];
      char index_entry[RANDSTR_LEN + TITLE_LEN];

      char title[TITLE_LEN];
      char password[PASS_LEN];
      char username[100];
      char url[200];
      char notes[1000];
      /* We initialize the seed for generating random strings. Now, this might be a shitty way to get a seed, but all
       * I want is some junk to put as a filename, it's not a mission critical task */
      srand(time(0));
      /* then generate such a random junk string, */
      rand_junk_str(rand_str, RANDSTR_LEN);
      /* and append it to the folder path, */
      strncat(file_path, rand_str, 290);

      fputs("Title: ", stdout);
      fgets(title, 100, stdin);

      fputs("Password: ", stdout);
      password_input(password);
      fputs("\n", stdout);

      fputs("Username: ", stdout);
      fgets(username, 100, stdin);

      fputs("URL: ", stdout);
      fgets(url, 200, stdin);

      fputs("Notes: ", stdout);
      fgets(notes, 1000, stdin);

      /* strlen() excludes the null byte, but strcat() includes it, so when allocating the entire
       * entry on the stack, we need to take that into account. We need 5 extra elements for the
       * null bytes at the end, and 4 new line characters. */
      unsigned int entry_len = strlen(title) + strlen(password) + strlen(username) + strlen(url) + strlen(notes) + 9;
      char entry[entry_len];
      strcpy(entry, title);
      strcat(entry, "\n");
      strcat(entry, password);
      strcat(entry, "\n");
      strcat(entry, username);
      strcat(entry, "\n");
      strcat(entry, url);
      strcat(entry, "\n");
      strcat(entry, notes);

      if (encrypt(file_path, entry, entry_len) != 0) {
        fputs("Unable to encrypt password file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }

      /* Now, we append the title of the entry and corresponding randomized filename to the end of the index file */
      size_t index_len = ((size_t)get_file_size(index_path) - crypto_secretbox_MACBYTES)/sizeof(char);
      char* index_buf = calloc(index_len, sizeof(char));
      if (decrypt(index_path, index_buf, index_len) != 0) {
        fputs("Unable to decrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
      strcpy(index_entry, rand_str);
      strcat(index_entry, ",");
      strcat(index_entry, title);
   }
   else {
     fputs("The index file doesn't exist. Please run \"citpass init\" to create it.\n", stdout);
   }
 }
 else {
   fputs("The folder at ", stdout);
   fputs(dir_path, stdout);
   fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
 }
}

void list_passwords() {
  char home_path[100];
  strncpy(home_path, getenv("HOME"), 100);
  char dir_path[200];
  setting_dir_path(home_path, dir_path);

  char index_path[300];
  strncpy(index_path, dir_path, 280);
  strncat(index_path, "/index", 280);
  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
    /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(index_path, F_OK) != -1) {
      /* We get the file size first, */
      size_t index_len = ((size_t)get_file_size(index_path) - crypto_secretbox_MACBYTES)/sizeof(char);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's filesize */
      char* index_buf = calloc(index_len, sizeof(char));
      /* Error handling */
      if (! index_buf) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        exit(EXIT_FAILURE);
      }
      /* Index file decryption */
      if (decrypt(index_path, index_buf, index_len) != 0) {
        fputs("Unable to decrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
      /* Let's first figure out how many lines we have in the index file, by counting the amount of newline characters, */
      unsigned int lines = 0;
      for (unsigned int n = 0; n < index_len; n++) {
        if (index_buf[n] == '\n') {
          lines++;
        }
      }
      /* Since we don't know how many passwords a user has stored in the folder,
       * it's necessary to dynamically allocate memory for the array that'll hold
       * the title strings. It's a 2D character array, so a 1D string array.
       * First, we allocate the first "column" */
      char** titles = calloc(lines, sizeof(char));
      /* Error handling */
      /* 0 is treated as a boolean false. Here, the ! indicates a reversal of that; if titles is null,
       * as in, 0, it's now treated as a boolean true, which means that we couldn't allocate memory,
       * so we error out */
      if (! titles) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
        free(titles);
        free(index_buf);
        exit(EXIT_FAILURE);
      }
      for (unsigned int n = 0; n < lines; n++) {
        titles[n] = calloc(TITLE_LEN, sizeof(char));
        /* Error handling */
        if (! titles[n]) {
          fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
          unsigned int m = n - 1;
          while (m >= 0) {
            free(titles[m]);
            m--;
          }
          /* Freeing memory twice is undefined behaviour... */
          free(titles);
          free(index_buf);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, index_buf, index_len);
      free(index_buf);
      /* Finally, we print out the list of titles */
      for (unsigned int n = 0; n < lines; n++) {
        fputs(titles[n], stdout);
        fputs("\n", stdout);
      }
      free(titles);
    }
    else {
      fputs("The index file doesn't exist. Please run \"citpass init\" to create it.\n", stdout);
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dir_path, stdout);
    fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
  }
}

void rm_password() {
  char home_path[100];
  strncpy(home_path, getenv("HOME"), 100);
  char dir_path[200];
  setting_dir_path(home_path, dir_path);

  char index_path[300];
  strncpy(index_path, dir_path, 280);
  strncat(index_path, "/index", 280);

  char file_path[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(file_path, dir_path, 280);
  strcat(file_path, "/");

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
    /* And here, we check if the index file within exists too. */
    if (access(index_path, F_OK) != -1) {
      /* We get the file size first, */
      size_t index_len = ((size_t)get_file_size(index_path) - crypto_secretbox_MACBYTES)/sizeof(char);
      /* Then we allocate memory for the unencrypted contents, */
      char* index_buf = calloc(index_len, sizeof(char));
      if (! index_buf) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        exit(EXIT_FAILURE);
      }
      /* Index file decryption */
      if (decrypt(index_path, index_buf, index_len) != 0) {
        fputs("Unable to decrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
      /* Let's first figure out how many lines we have in the file, by counting the amount of newline characters, */
      unsigned int lines = 0;
      for (unsigned int n = 0; n < index_len; n++) {
        if (index_buf[n] == '\n') {
          lines++;
        }
      }
      char** titles = calloc(lines, sizeof(char));
      /* Error handling */
      if (! titles) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
        free(titles);
        free(index_buf);
        exit(EXIT_FAILURE);
      }
      for (unsigned int n = 0; n < lines; n++) {
        titles[n] = calloc(TITLE_LEN, sizeof(char));
        /* Error handling */
        if (! titles[n]) {
          fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
          unsigned int m = n - 1;
          while (m >= 0) {
            free(titles[m]);
            m--;
          }
          /* Freeing memory twice is undefined behaviour... */
          free(titles);
          free(index_buf);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, index_buf, index_len);
      free(index_buf);
      /* Title printing to stdout */
      for (unsigned int n = 1; n < lines; n++) {
        fputs(titles[n], stdout);
        fputs("\n", stdout);
      }
      free(titles);
      /* User selects entry */
      /* Index file decryption */
      /* Entry is deleted from index file */
      /* Index file encryption */
      /* Selected password file is deleted */
   }
    else {
      fputs("The index file doesn't exist. Please run \"citpass init\" to create it.\n", stdout);
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dir_path, stdout);
    fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
  }
}

void get_password() {
  char home_path[100];
  strncpy(home_path, getenv("HOME"), 100);
  char dir_path[200];
  setting_dir_path(home_path, dir_path);

  char index_path[300];
  strncpy(index_path, dir_path, 280);
  strncat(index_path, "/index", 280);

  char file_path[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done later */
  strncpy(file_path, dir_path, 280);
  strcat(file_path, "/");

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
   /* And here, we check if the index file within exists too. */
    if (access(index_path, F_OK) != -1) {
      /* We get the file size first, which needs to be subtracted by the fixed number of extra bytes, in order to get
       * the actual size of the unencrypted contents */
      size_t index_len = ((size_t)get_file_size(index_path) - crypto_secretbox_MACBYTES)/sizeof(char);
      /* File's characters will be stored at this pointer below, we're setting index_buf up to point
       * at the allocated memory with the index file's size */
      char* index_buf = calloc(index_len, sizeof(char));
      /* Error handling */
      if (! index_buf) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        exit(EXIT_FAILURE);
      }
      /* Index file decryption */
      if (decrypt(index_path, index_buf, index_len) != 0) {
        fputs("Unable to decrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
      unsigned int lines = 0;
      for (unsigned int n = 0; n < index_len; n++) {
        if (index_buf[n] == '\n') {
          lines++;
        }
      }
      char** titles = calloc(lines, sizeof(char));
      if (! titles) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
        free(titles);
        free(index_buf);
        exit(EXIT_FAILURE);
      }
      for (unsigned int n = 0; n < lines; n++) {
        titles[n] = calloc(TITLE_LEN, sizeof(char));
        if (! titles[n]) {
          fputs("Failed to allocate needed memory for reading index file. Aborting.\n", stdout);
          unsigned int m = n - 1;
          while (m >= 0) {
            free(titles[m]);
            m--;
          }
          /* Freeing memory twice is undefined behaviour... */
          free(titles);
          free(index_buf);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, index_buf, index_len);
      free(index_buf);
      /* Print out the list of titles */
      for (unsigned int n = 0; n < lines; n++) {
        fputs(titles[n], stdout);
        fputs("\n", stdout);
      }
      free(titles);
      /* User selects entry */
      /* Password file decryption */
      size_t file_len = ((size_t)get_file_size(file_path) - crypto_secretbox_MACBYTES)/sizeof(char);
      char* file_buf = calloc(file_len, sizeof(char));
      if (decrypt(file_path, file_buf, file_len) != 0) {
        fputs("Unable to decrypt password file. Aborting.\n", stdout);
        free(file_buf);
        exit(EXIT_FAILURE);
      }
      /* Password is printed to stdout/piped to clipboard manager */
      free(file_buf);
      /* Password file encryption */
    }
   else {
     fputs("The index file doesn't exist. Please run \"citpass init\" to create it.\n", stdout);
   }
 }
 else {
   fputs("The folder at ", stdout);
   fputs(dir_path, stdout);
   fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
 }
}

int main(int argc, char *argv[]) {
  if (sodium_init() < 0) {
    /* Library couldn't be initialized, it is not safe to use */
    fputs("File encryption is not available. Aborting.\n", stdout);
    exit(EXIT_FAILURE);
  }
  /* Now, it's necessary to parse the command passed to the program, so */
  if (argc == 1) {
    /* This first case below executes when just the binary's name has been invoked.
     * Can't declare and define variables within the switch statement, so gotta check
     * early if there's just the executable name, that way we don't check argv[1]
     * if it doesn't exist, we can't do that since it's undefined behaviour */
    show_command_information(0);
    exit(EXIT_SUCCESS);
  }
  int init = strncmp(argv[1], "init", 5);
  int add = strncmp(argv[1], "add", 5);
  int ls = ((strncmp(argv[1], "ls", 5) == 0) || (strncmp(argv[1], "list", 5) == 0) || (strncmp(argv[1], "show", 5) == 0)) ? 0 : 1;
  int rm = strncmp(argv[1], "rm", 5);
  int get = strncmp(argv[1], "get", 5);
  /* First, it's necessary to know how many commands have been passed */
  switch (argc) {
  case 2:
    /* Here's the case when a command's been passed to the program, */
    if (init == 0) {
      initialize();
    }
    else if (add == 0) {
      add_password();
    }
    else if (ls == 0) {
      list_passwords();
    }
    else if (rm == 0) {
      rm_password();
    }
    else if (get == 0) {
      get_password();
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
      show_command_information(2);
    }
    else if (get == 0) {
      show_command_information(2);
    }
    else {
      show_command_information(1);
    }
    break;
  default:
    show_command_information(4);
  }
  return EXIT_SUCCESS;
}
