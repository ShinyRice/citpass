#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  /* First, it's necessary to know how many arguments have been passed. This first case below
  * executes when just the binary's name has been invoked, */
  if(argc == 1) {
    printf("citpass requires an argument. Possible arguments are:\n");
    printf("init - Create the file where passwords will be stored, located at $HOME/.local/share/citpass/passwords\n");
    printf("add - Add a password along with associated information to said file\n");
    printf("list - List all entries for which there is a password\n");
    printf("rm - Remove a password along with associated information from the file\n");
    printf("get - Retrieve a password from the file\n");
  }
  else if(argc == 2){
    /* This is the case where the binary's name plus another argument has been passed.
    * Now, it's necessary to parse the command line argument passed to the program, so */
    int arginit = strncmp(argv[1], "init", 5);
    int argadd = strncmp(argv[1], "add", 5);
    int arglist = strncmp(argv[1], "list", 5);
    int argrm = strncmp(argv[1], "rm", 5);
    int argget = strncmp(argv[1], "get", 5);

    if (arginit == 0) {
      /* Initialization
      * Before doing anything, we need to know whether or not the application folder in ~/.local/share
      * and the file within exists. If both exist, then nothing is done. If the folder exists, but the file doesn't,
      */
      if(access("/home/shinyrice/.local/share/citpass", F_OK) != -1) {
        printf("The folder at /home/shinyrice/.local/share/citpass exists.\n");
      }
      else {
        printf("The folder at /home/shinyrice/.local/share/citpass doesn't exist.\n");
        printf("Creating it.");
        /* Empty file creation */
      }

    /* File encryption */

    }
    else if(argadd == 0) {
    /* Addition of password */

      printf("Placeholder for add.\n");

    /* File unencryption */

    /* Appending an empty entry to the end of the database file */

    /* User now fills the entry with information */

    /* File encryption */

    }
    else if(arglist == 0) {
    /* Addition of password */

      printf("Placeholder for list.\n");

    /* File unencryption */

    /* Appending an empty entry to the end of the database file */

    /* User now fills the entry with information */

    /* File encryption */

    }
    else if(argrm == 0) {
    /* Removal of password */

      printf("Placeholder for rm.\n");

    /* File unencryption */

    /* Print out list of entries */

    /* User selects entry */

    /* Entry is deleted */

    /* File encryption */

    }
    else if(argget == 0) {
    /* Retrieval of password */

      printf("Placeholder for get.\n");

    /* File unencryption */

    /* Print out list of entries */

    /* User selects entry */

    /* Password is printed to stdout */

    /* File encryption */

    }
    else {
      printf("Invalid argument. Please provide a valid one.\n");
      printf("Possible arguments are:\n");
      printf("init - Create the file where passwords will be stored, located at $HOME/.local/share/citpass/passwords\n");
      printf("add - Add a password along with associated information to said file\n");
      printf("list - List all entries for which there is a password\n");
      printf("rm - Remove a password along with associated information from the file\n");
      printf("get - Retrieve a password from the file\n");
    }
  }
  else if(argc > 2) {
    /* citpass will require a second argument when retrieving and removing a password, the title of such a password.
    * As such, it'll be here when the time comes. */
      printf("Too many arguments supplied.\n");
  }
  return 0;
}
