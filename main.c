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
#define PAD_BYTES 40

/* In order to avoid getting a nice fat warning, a quick workaround is to declare fileno() explicitly in the source
 * I wrote, instead of having that done in the corresponding header file...
 * https://stackoverflow.com/questions/46213840/get-rid-of-warning-implicit-declaration-of-function-fileno-in-flex */
int fileno(FILE *stream);

/* Functions */
void show_command_information(const int sit) {
  switch (sit) {
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

/* Parsing possible commands for listing passwords */
unsigned int parse_ls(const char* list) {
  unsigned int result = 1;
  if ((strncmp(list, "ls", 5) == 0) || (strncmp(list, "list", 5) == 0) || (strncmp(list, "show", 5) == 0)) {
    result = 0;
  }
  else {
    result = 1;
  }
  return result;
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
off_t get_file_size(FILE* fp) {
  /* It's necessary to find out the size of the file.
  * In order to do that, first we find out the file descriptor, fd, */
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
  return file_size;
}

size_t read_index_file(FILE* index_fp, char* buffer) {
  /* We write the file to memory. getc() reads one character at a time,
   * every time it is called it reads the next character */
  char c;
  unsigned int n = 0;
  /* Worth noting, at least for myself, that the condition in this loop means the buffered index file
   * won't have an EOF character, so no need to deal with it. */
  while ((c = getc(index_fp)) != EOF) {
    buffer[n] = c;
    n++;
  }
  size_t buf_len = n + 1;
  return buf_len;
}

/* The way this function parses the index file puts the entire second column of the CSV index file
 * into titles[], which means that "Titles" will invariably be the first string in that array.
 * We won't ignore that when parsing, but we do ignore it when printing titles[], simply by starting
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

/* Take the first line of the index, "Filename,Title", and encrypt that, then put it in the index file.
 * This function's specifically for encrypting the index file for the first time */
static int init_encrypt_index_file(const char* dest_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]) {
  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  unsigned char buf_out[15 + crypto_secretstream_xchacha20poly1305_ABYTES];
  crypto_secretstream_xchacha20poly1305_state st;
  unsigned long long out_len;
  const unsigned char first_line[] = "Filename,Title";
  size_t line_len = strlen((char*)first_line);

  crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
  crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, first_line, line_len, NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL);
  FILE* dest_fp = fopen(dest_file, "wb");
  if (! dest_fp) {
    puts("Failed to create index file. Aborting.");
    exit(EXIT_FAILURE);
  }
  fwrite(header, 1, sizeof(header), dest_fp);
  fwrite(buf_out, 1, out_len, dest_fp);
  fclose(dest_fp);
  return 0;
}

void init_create_index_file(const char* index_path) {
  puts("Creating index file within folder.");
  char mast_pass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mast_pass);

  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];

  randombytes_buf(salt, sizeof(salt));
  fputs("\n", stdout);
  if (crypto_pwhash(key, sizeof(key), mast_pass, strlen(mast_pass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    puts("Ran out of memory while deriving key from master password. Aborting.");
    exit(EXIT_FAILURE);
  }
  if (init_encrypt_index_file(index_path, key) != 0) {
    puts("Failed to encrypt index file. Aborting.");
    exit(EXIT_FAILURE);
  }
}

static int encrypt(const char* dest_file_path, const unsigned char* message, const size_t message_len) {
  unsigned char ciphertext[message_len + crypto_secretbox_MACBYTES];
  char mast_pass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mast_pass);
  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[crypto_secretbox_KEYBYTES];

  randombytes_buf(salt, sizeof(salt));
  fputs("\n", stdout);
  if (crypto_pwhash(key, sizeof(key), mast_pass, strlen(mast_pass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    puts("Ran out of memory while deriving key from master password. Aborting.");
    exit(EXIT_FAILURE);
  }

  crypto_secretbox_easy(ciphertext, message, message_len, salt, key);
  FILE* dest_fp = fopen(dest_file_path, "wb");
  if (! dest_fp) {
    puts("Failed to open file. Aborting.");
    exit(EXIT_FAILURE);
  }
  fwrite(ciphertext, 1, (size_t) message_len + crypto_secretbox_MACBYTES, dest_fp);
  fclose(dest_fp);
  return 0;
}

static int decrypt(const char* src_file_path) {
  char mast_pass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mast_pass);

  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[crypto_secretbox_KEYBYTES];

  randombytes_buf(salt, sizeof(salt));
  fputs("\n", stdout);
  if (crypto_pwhash(key, sizeof(key), mast_pass, strlen(mast_pass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    puts("Ran out of memory while deriving key from master password. Aborting.");
    exit(EXIT_FAILURE);
  }
  FILE* dest_fp = fopen(src_file_path, "rb");
  if (! dest_fp) {
    puts("Failed to open file. Aborting.");
    exit(EXIT_FAILURE);
  }
  /* It's necessary to know how much memory to allocate to get the file onto memory.
   * In order to do that, we need the file size; but most crypto algos usually add
   * a fixed number of bytes to the file. I'm going to take advantage of this fact. */
  size_t message_len = get_file_size(dest_fp) - crypto_secretbox_MACBYTES;
  unsigned char* message = calloc(message_len, sizeof(char));
  unsigned char ciphertext[message_len + crypto_secretbox_MACBYTES];
  fread(ciphertext, sizeof(char), message_len + crypto_secretbox_MACBYTES, dest_fp);
  if (crypto_secretbox_open_easy(message, ciphertext, message_len + crypto_secretbox_MACBYTES, salt, key) != 0) {
    puts("File contents have been forged or corrupted. Aborting.");
    exit(EXIT_FAILURE);
  }
  fclose(dest_fp);
  return 0;
}

/* Checking whether or not password directory exists. If it doesn't, it's created, then the index file is created
 * within and encrypted. If it does, we check for the existence of the index file, and if it doesn't exist,
 * we create it and encrypt it. */
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
      init_create_index_file(index_path);
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dir_path, stdout);
    fputs(" doesn't exist. Creating it.\n", stdout);
    if (mkdir(dir_path, 0700) == -1) {
      puts("Creating folder failed. Aborting.");
      exit(EXIT_FAILURE);
    }
    else {
      init_create_index_file(index_path);
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

  char rand_str[RANDSTR_LEN];
  char index_entry[RANDSTR_LEN + TITLE_LEN];

  char title[TITLE_LEN];
  char password[PASS_LEN];
  char username[100];
  char url[200];
  char notes[1000];

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(index_path, F_OK) != -1) {
      /* We initialize the seed for generating random strings. Now, this might be a shitty way to get a seed, but all
       * I want is some junk to put as a filename, it's not a mission critical task */
      srand(time(0));
      /* then generate such a random junk string, */
      rand_junk_str(rand_str, 50);
      /* and append it to the folder path, */
      strncat(file_path, rand_str, 290);

      /* File is opened, and subsequently the user fills the file with the password and relevant metadata */
      FILE* entry_fp = fopen(file_path, "a");
      if (! entry_fp) {
        puts("Failed to create password entry file. Aborting.");
        exit(EXIT_FAILURE);
      }

      fputs("Title: ", stdout);
      fgets(title, 100, stdin);
      fputs("Title: ", entry_fp);
      fputs(title, entry_fp);

      fputs("Password: ", stdout);
      password_input(password);
      fputs("Password: ", entry_fp);
      fputs(password, entry_fp);
      fputs("\n", stdout);

      fputs("Username: ", stdout);
      fgets(username, 100, stdin);
      fputs("Username: ", entry_fp);
      fputs(username, entry_fp);

      fputs("URL: ", stdout);
      fgets(url, 200, stdin);
      fputs("URL: ", entry_fp);
      fputs(url, entry_fp);

      fputs("Notes: ", stdout);
      fgets(notes, 1000, stdin);
      fputs("Notes: ", entry_fp);
      fputs(notes, entry_fp);

      fclose(entry_fp);

      /* File's closed, and thus now we append the title of the entry and corresponding randomized
       * filename to the end of the index file */
      FILE* index_fp = fopen(index_path, "a");
      if (! index_fp) {
        puts("Failed to open index file. Aborting.");
        exit(EXIT_FAILURE);
      }
      strncpy(index_entry, rand_str, 140);
      strncat(index_entry, ",", 140);
      strncat(index_entry, title, 140);
      fputs(index_entry, index_fp);
      fclose(index_fp);
   }
   else {
     puts("The index file doesn't exist. Please run \"citpass init\" to create it.");
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
      /* Index file decryption */
      FILE* index_fp = fopen(index_path, "r");
      /* Error handling */
      if (! index_fp) {
        puts("Failed to open index file. Aborting.");
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(index_fp);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's filesize */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(index_fp, buffer);
      fclose(index_fp);
      /* Let's first figure out how many lines we have in the file, by counting the amount of newline characters, */
      unsigned int lines = 0;
      for (unsigned int n = 0; n < buf_len; n++) {
        if (buffer[n] == '\n') {
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
        puts("Failed to allocate needed memory for reading index file. Aborting.");
        free(titles);
        free(buffer);
        exit(EXIT_FAILURE);
      }
      for (unsigned int n = 0; n < lines; n++) {
        titles[n] = calloc(TITLE_LEN, sizeof(char));
        /* Error handling */
        if (! titles[n]) {
          puts("Failed to allocate needed memory for reading index file. Aborting.");
          unsigned int m = n - 1;
          while (m >= 0) {
            free(titles[m]);
            m--;
          }
          /* Freeing memory twice is undefined behaviour... */
          free(titles);
          free(buffer);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, buffer, buf_len);
      free(buffer);
      /* Finally, we print out the list of titles */
      for (unsigned int n = 1; n < lines; n++) {
        fputs(titles[n], stdout);
        fputs("\n", stdout);
      }
      free(titles);
      /* Index file encryption */
    }
    else {
      puts("The index file doesn't exist. Please run \"citpass init\" to create it.");
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
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done
   * according to user input */
  strncpy(file_path, dir_path, 280);
  strcat(file_path, "/");
  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
    /* And here, we check if the index file within exists too. */
    if (access(index_path, F_OK) != -1) {
      /* Index file decryption */
      if (decrypt(index_path) != 0) {
        fputs("Unable to decrypt index file. Aborting.\n", stdout);
        exit(EXIT_FAILURE);
      }
      FILE* index_fp = fopen(index_path, "r");
      /* Error handling */
      if (! index_fp) {
        puts("Failed to open index file. Aborting.");
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(index_fp);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's filesize */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(index_fp, buffer);
      fclose(index_fp);

      /* Let's first figure out how many lines we have in the file, by counting the amount of newline characters, */
      unsigned int lines = 0;
      for (unsigned int n = 0; n < buf_len; n++) {
        if (buffer[n] == '\n') {
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
        puts("Failed to allocate needed memory for reading index file. Aborting.");
        free(titles);
        free(buffer);
        exit(EXIT_FAILURE);
      }
      for (unsigned int n = 0; n < lines; n++) {
        titles[n] = calloc(TITLE_LEN, sizeof(char));
        /* Error handling */
        if (! titles[n]) {
          puts("Failed to allocate needed memory for reading index file. Aborting.");
          unsigned int m = n - 1;
          while (m >= 0) {
            free(titles[m]);
            m--;
          }
          /* Freeing memory twice is undefined behaviour... */
          free(titles);
          free(buffer);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, buffer, buf_len);
      free(buffer);
      /* Finally, we print out the list of titles */
      for (unsigned int n = 1; n < lines; n++) {
        fputs(titles[n], stdout);
        fputs("\n", stdout);
      }
      free(titles);
      /* Index file encryption */
      /* Print out list of entries */
      /* User selects entry */
      /* Entry is deleted from index file */
      /* Index file encryption */
      /* Selected password file is deleted */
   }
    else {
      puts("The index file doesn't exist. Please run \"citpass init\" to create it.");
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
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done
   * according to user input */
  strncpy(file_path, dir_path, 280);
  strcat(file_path, "/");

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dir_path, F_OK) != -1) {
   /* And here, we check if the index file within exists too. */
    if (access(index_path, F_OK) != -1) {
      /* Master password put in by user */
      /* Key generated from master password */
      /* Index file decryption */
      FILE* index_fp = fopen(index_path, "r");
      /* Error handling */
      if (! index_fp) {
        puts("Failed to open index file. Aborting.");
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(index_fp);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's size */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(index_fp);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(index_fp, buffer);
      free(buffer);
      fclose(index_fp);
      /* Print out list of entries */
      /* Index file encryption */
      /* User selects entry */
      /* Password file decryption */
      /* Password is printed to stdout/piped to clipboard manager */
      /* Password file encryption */
    }
   else {
     puts("The index file doesn't exist. Please run \"citpass init\" to create it.");
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
    /* The library couldn't be initialized, it is not safe to use */
    puts("File encryption is not available. Aborting.");
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
  int ls = parse_ls(argv[1]);
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
      rm_password();
    }
    else if (get == 0) {
      get_password();
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
