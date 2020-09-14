/* Forgive the excess of obvious comments in this source. They somewhat ease subsequent readings of the code
 * that I make, to consolidate what I learnt already. */

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

#define CHUNK_SIZE 4096
#define RANDSTR_LEN 50
#define TITLE_LEN 100
#define PASS_LEN 200
#define KEY_LEN 16

/* In order to avoid getting a nice fat warning, a quick workaround is to declare fileno() explicitly in the source
 * I wrote, instead of having that done in the corresponding header file...
 * https://stackoverflow.com/questions/46213840/get-rid-of-warning-implicit-declaration-of-function-fileno-in-flex */
int fileno(FILE *stream);

/* Functions */
/* Setting the path to the directory where passwords are stored, done through an environment variable */
void setting_dirpath(const char* homepath, char* dirpath) {
  char* storelocation = getenv("CITPASS_DIR");
  /* If storelocation is null, it'll be 0. It's a pointer pointing to nothing, in other words, the environment
   * variable hasn't been set and is just an empty string, and so we jump to the else case, since 0 is a boolean false.
   * If it's not empty we use it as the password folder location */
  if (storelocation) {
    strncpy(dirpath, storelocation, 200);
  }
  else {
    strncpy(dirpath, homepath, 200);
    strncat(dirpath, "/.local/share/citpass", 200);
  }
}

/* Parsing possible commands for listing passwords */
int parse_ls(const char* list) {
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
  * In order to do that, first we find out the file descriptor, (fd), */
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
  int statresult = fstat(fd, &buf);
  /* Error handling */
  if ((statresult != 0) || (!S_ISREG(buf.st_mode))) {
    fputs("Could not read index file. Aborting.", stdout);
    fclose(fp);
    exit(EXIT_FAILURE);
  }

  off_t filesize = buf.st_size;
  /* I'll set a large upper limit for the file, 1 MB. */
  if (filesize > 1000000) {
    fputs("Index file is larger than 1 MB. Aborting.", stdout);
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  return filesize;
}

size_t read_index_file(FILE* indexfile, char* buffer) {
  /* We write the file to memory. getc() reads one character at a time,
   * every time it is called it reads the next character */
  char c;
  unsigned int n = 0;
  /* Worth noting, at least for myself, that the condition in this loop means the buffered index file
   * won't have an EOF character, so no need to deal with it. */
  while ((c = getc(indexfile)) != EOF) {
    buffer[n] = c;
    n++;
  }
  size_t buf_len = n + 1;
  return buf_len;
}

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
static int init_encrypt_index_file(const char *dest_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]) {
  unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
  unsigned char buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
  crypto_secretstream_xchacha20poly1305_state st;
  unsigned long long out_len;
  const unsigned char first_line[] = "Filename,Title";
  size_t line_len = strlen((char*)first_line);

  crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
  crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, first_line, line_len, NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_FINAL);
  FILE* fp_dest = fopen(dest_file, "wb");
  fwrite(header, 1, sizeof header, fp_dest);
  fwrite(buf_out, 1, out_len, fp_dest);
  fclose(fp_dest);
  return 0;
}

void init_create_index_file(char* indexpath) {
  puts("Creating index file within folder as well.");
  char mastpass[PASS_LEN];
  fputs("Master password: ", stdout);
  password_input(mastpass);

  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char key[KEY_LEN];

  randombytes_buf(salt, sizeof salt);
  if (crypto_pwhash(key, sizeof key, mastpass, strlen(mastpass), salt, crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT) != 0) {
    puts("Ran out of memory while deriving key from master password. Aborting.");
    exit(EXIT_FAILURE);
  }
  if (init_encrypt_index_file(indexpath, key) != 0) {
    puts("Failed to encrypt index file. Aborting.");
    exit(EXIT_FAILURE);
  }
}

/* Checking whether or not password directory exists. If it doesn't, it's created, then the index file is created
 * within and encrypted. If it does, we check for the existence of the index file, and if it doesn't exist,
 * we create it and encrypt it. */
void initalization() {
  char homepath[100];
  strncpy(homepath, getenv("HOME"), 100);
  char dirpath[200];
  setting_dirpath(homepath, dirpath);

  char indexpath[300];
  strncpy(indexpath, dirpath, 280);
  strncat(indexpath, "/index", 280);

  if (access(dirpath, F_OK) != -1) {
    fputs("The folder at ", stdout);
    fputs(dirpath, stdout);
    fputs(" exists.\n", stdout);
    if (access(indexpath, F_OK) != -1) {
      fputs("The index file at ", stdout);
      fputs(indexpath, stdout);
      fputs(" exists as well. No action necessary.\n", stdout);
    }
    else {
      init_create_index_file(indexpath);
    }
  }
  else {
    fputs("The folder at ", stdout);
    fputs(dirpath, stdout);
    fputs(" doesn't exist. Creating it.\n", stdout);
    if (mkdir(dirpath, 0700) == -1) {
      puts("Creating folder failed. Aborting.");
      exit(EXIT_FAILURE);
    }
    else {
      init_create_index_file(indexpath);
    }
  }
}

/* Adding a password to the folder, and adding the random filename to the index */
/* filepath can be a local variable to this function just fine, no need to have
 * it defined in main. */
void add_password() {
  char homepath[100];
  strncpy(homepath, getenv("HOME"), 100);
  char dirpath[200];
  setting_dirpath(homepath, dirpath);

  char indexpath[300];
  strncpy(indexpath, dirpath, 280);
  strcat(indexpath, "/index");

  char filepath[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(filepath, dirpath, 280);
  strcat(filepath, "/");

  char randstr[RANDSTR_LEN];
  char title[TITLE_LEN];
  char password[PASS_LEN];
  char username[100];
  char url[200];
  char notes[1000];
  char indexentry[150];

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dirpath, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* We initialize the seed for generating random strings.
       * Now, this might be a shitty way to get a seed, but all
       * I want is some junk to put as a filename, it's not a
       * mission critical task */
      srand(time(NULL));
      /* then generate such a random junk string, */
      rand_junk_str(randstr, 50);
      /* and append it to the folder path, */
      strncat(filepath, randstr, 290);

      /* File is opened, and subsequently the user fills the file with the password and relevant metadata */
      FILE* fileadd = fopen(filepath, "a");
      fputs("Title: ", stdout);
      fgets(title, 100, stdin);
      fputs("Title: ", fileadd);
      fputs(title, fileadd);

      fputs("Password: ", stdout);
      password_input(password);
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
      FILE* indexadd = fopen(indexpath, "a");
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

void list_passwords() {
  char homepath[100];
  strncpy(homepath, getenv("HOME"), 100);
  char dirpath[200];
  setting_dirpath(homepath, dirpath);

  char indexpath[300];
  strncpy(indexpath, dirpath, 280);
  strncat(indexpath, "/index", 280);
  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dirpath, F_OK) != -1) {
    /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* Index file decryption */
      FILE* indexfile = fopen(indexpath, "r");
      /* Error handling */
      if (! indexfile) {
        puts("Failed to open index file. Aborting.");
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(indexfile);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's filesize */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(indexfile, buffer);
      fclose(indexfile);
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
          titles[0] = 0;
          free(titles);
          free(buffer);
          exit(EXIT_FAILURE);
        }
      }
      parse_index_file(titles, lines, buffer, buf_len);
      free(buffer);
      /* Finally, we print out the list of titles */
      for (unsigned int n = 0; n < lines; n++) {
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
    fputs(dirpath, stdout);
    fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
  }
}

void rm_password() {
  char homepath[100];
  strncpy(homepath, getenv("HOME"), 100);
  char dirpath[200];
  setting_dirpath(homepath, dirpath);

  char indexpath[300];
  strncpy(indexpath, dirpath, 280);
  strncat(indexpath, "/index", 280);

  char filepath[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(filepath, dirpath, 280);
  strcat(filepath, "/");
  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dirpath, F_OK) != -1) {
    /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* Index file decryption */
      FILE* indexfile = fopen(indexpath, "r");
      /* Error handling */
      if (! indexfile) {
        puts("Failed to open index file. Aborting.");
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(indexfile);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's filesize */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(indexfile, buffer);
      fclose(indexfile);

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
      for (unsigned int n = 0; n < lines; n++) {
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
    fputs(dirpath, stdout);
    fputs(" doesn't exist. Please run \"citpass init\" to create both it and the index file within.\n", stdout);
  }
}

void get_password() {
  char homepath[100];
  strncpy(homepath, getenv("HOME"), 100);
  char dirpath[200];
  setting_dirpath(homepath, dirpath);

  char indexpath[300];
  strncpy(indexpath, dirpath, 280);
  strncat(indexpath, "/index", 280);

  char filepath[300];
  /* The full path of the file to be decrypted and opened isn't completely specified, that will be done according to user input */
  strncpy(filepath, dirpath, 280);
  strcat(filepath, "/");

  /* Here, we check if the folder where passwords are stored exists, */
  if (access(dirpath, F_OK) != -1) {
   /* And here, we check if the index file within exists too. In this case, since they both exist, we do the deed. */
    if (access(indexpath, F_OK) != -1) {
      /* Index file decryption */
      FILE* indexfile = fopen(indexpath, "r");
      /* Error handling */
      if (! indexfile) {
        puts("Failed to open index file. Aborting.");
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      /* We get the file size first, */
      off_t file_size = get_file_size(indexfile);
      /* File's characters will be stored at this pointer below, we're setting buffer up to point
       * at the allocated memory with the index file's size */
      char* buffer = calloc(file_size/sizeof(char), sizeof(char));
      /* Error handling */
      if (! buffer) {
        fputs("Failed to allocate needed memory for reading index file. Aborting.", stdout);
        fclose(indexfile);
        exit(EXIT_FAILURE);
      }
      size_t buf_len = read_index_file(indexfile, buffer);
      free(buffer);
      fclose(indexfile);
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
   fputs(dirpath, stdout);
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
  int init = strncmp(argv[1], "init", 5);
  int add = strncmp(argv[1], "add", 5);
  int ls = parse_ls(argv[1]);
  int rm = strncmp(argv[1], "rm", 5);
  int get = strncmp(argv[1], "get", 5);
  /* First, it's necessary to know how many commands have been passed */
  switch (argc) {
  case 1:
    /* This first case below executes when just the binary's name has been invoked, */
    show_command_information(0);
    break;
  case 2:
    /* Here's the case when a command's been passed to the program, */
    if (init == 0) {
      initalization();
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
  return 0;
}
