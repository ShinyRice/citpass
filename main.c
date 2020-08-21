#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int parsels(char* list);

int main(int argc, char *argv[])
{
  char* homepath = getenv("HOME");
  /* First, it's necessary to know how many arguments have been passed. This first case below
  * executes when just the binary's name has been invoked, */
  if (argc == 1)
  {
    printf("citpass requires a command. Possible arguments are:\n");
    printf("init - Create the file where passwords will be stored, located at $HOME/.local/share/citpass/passwords\n");
    printf("add - Add a password along with associated information to said file\n");
    printf("ls - List all entries for which there is a password\n");
    printf("rm - Remove a password along with associated information from the file\n");
    printf("get - Retrieve a password\n");
  }
  else if (argc == 2)
  {
    /* This is the case where the binary's name plus another argument has been passed.
    * Now, it's necessary to parse the command line argument passed to the program, so */
    int arginit = strncmp(argv[1], "init", 5);
    int argadd = strncmp(argv[1], "add", 5);
    int argls = parsels(argv[1]);
    int argrm = strncmp(argv[1], "rm", 5);
    int argget = strncmp(argv[1], "get", 5);

    char* folderpath = strncat(homepath, "/.local/share/citpass", 400);
    printf(folderpath);
    char* filepath = strncat(folderpath, "/passwords", 400); /* In the future, this'll be set by the user, through a configuration file or an argument */

    if (arginit == 0) {
      /* Initialization
      * Before doing anything, we need to know whether or not the application folder in ~/.local/share
      * and the file within exists. If both exist, then nothing is done. If the folder exists, but the file doesn't,
      * only the file is created. If the folder doesn't exist, then both the folder and the file within are created.
      */

      /* Here, we check if the folder exists, */
      if (access(folderpath, F_OK) != -1) {
        printf("The folder at %s exists.\n", folderpath);
        if (access(filepath, F_OK) != -1) {
          /* And here, we check if the file within exists as well, */
          printf("The file at %s exists as well. No action necessary.\n", filepath);
        }
        else {
          /* This is the case where the folder exists, but the file doesn't. The file is promptly created. */
          printf("The file doesn't exist. Creating it.\n");

          FILE *filecheck;
          filecheck = fopen(filepath, "w");
          fclose(filecheck);
        }
      }
      else {
        printf("The folder at %s doesn't exist. Creating it.\n", folderpath);

        if (mkdir(folderpath, 0600) == -1) {
          printf("Creating folder failed. Aborting.\n");
        }
        else {
          printf("Creating file within folder as well.\n");

          FILE *filecheck;
          filecheck = fopen(filepath, "w");
          fclose(filecheck);
        }
      }

    /* File encryption */

    }
    else if (argadd == 0) {
      /* Addition of password */

      /* Again, before doing anything, we need to know whether or not the application folder in ~/.local/share
      * and the file within exists. If both exist, then nothing is done. If the folder exists, but the file doesn't,
      * only the file is created. If the folder doesn't exist, then both the folder and the file within are created.
      */

      /* Here, we check if the folder exists, */
      if (access(folderpath, F_OK) != -1) {
        /* And here, we check if the file within exists as well. In this case, since they both exist, we do the deed. */
        if (access(filepath, F_OK) != -1) {
          char title[100];
          char password[100];
          char username[100];
          char url[100];
          char notes[100];

          FILE *fileadd;
          fileadd = fopen(filepath, "a");

          printf("Title:");
          scanf("%s", title);

          printf("\nPassword:");
          scanf("%s", password);

          printf("\nUsername:");
          scanf("%s", username);

          printf("\nURL:");
          scanf("%s", url);

          printf("\nNotes:");
          scanf("%s", notes);

          fprintf(fileadd, strncat("Title: ", title, 250));
          fprintf(fileadd, strncat("\nPassword: ", password, 250));
          fprintf(fileadd, strncat("\nUsername: ", username, 250));
          fprintf(fileadd, strncat("\nURL: ", url, 250));
          fprintf(fileadd, strncat("\nNotes: ", notes, 250));

          fclose(fileadd);
        }
        else {
          /* This is the case where the folder exists, but the file doesn't. The program asks the user to first go through init. */
          printf("The database file doesn't exist. Please run \"citpass init\" to create it.\n");
        }
      }
      else {
        printf("The folder at %s doesn't exist. Please run \"citpass init\" to create both it and the database file within.\n", folderpath);
      }

      /* File unencryption */

      /* User fills the entry with information */

      /* Appending an entry to the end of the database file */

      /* File encryption */

    }
    else if (argls == 0) {
      /* Addition of password */

      FILE *filels;
      filels = fopen(filepath, "r");

      /* File unencryption */

      /* Appending an empty entry to the end of the database file */

      /* User now fills the entry with information */

      fclose(filels);

      /* File encryption */

    }
    else if (argrm == 0) {
      /* Removal of password */

      FILE *filerm;
      filerm = fopen(filepath, "rw");

      /* File unencryption */

      /* Print out list of entries */

      /* User selects entry */

      /* Entry is deleted */

      fclose(filerm);

      /* File encryption */

    }
    else if (argget == 0) {
      /* Retrieval of password */

      FILE *fileget;
      fileget = fopen(filepath, "r");

      /* File unencryption */

      /* Print out list of entries */

      /* User selects entry */

      /* Password is printed to stdout */

      fclose(fileget);

      /* File encryption */

    }
    else {
      printf("Invalid command, please provide a valid one.\n");
      printf("Possible commands are:\n");
      printf("init - Create the file where passwords will be stored, located at $HOME/.local/share/citpass/passwords\n");
      printf("add - Add a password along with associated information to said file\n");
      printf("ls - List all entries for which there is a password\n");
      printf("rm - Remove a password along with associated information from the file\n");
      printf("get - Retrieve a password\n");
    }
  }
  else if (argc > 2) {
    /* citpass will require a second argument when retrieving and removing a password, the title of such a password.
    * As such, it'll be here when the time comes. */
    printf("Too many arguments supplied.\n");
  }
  return 0;
}

int parsels(char* list) {
  int result;

  if (strncmp(list, "ls", 5) == 0) {
    result = 0;
  }
  else if (strncmp(list, "list", 5) == 0) {
    result = 0;
  }
  else if (strncmp(list, "show", 5) == 0) {
    result = 0;
  }
  else {
    result = 1;
  }

  return result;
}
