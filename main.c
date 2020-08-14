#include <stdio.h>

int main(int argc, char *argv[])
{
  // First, it's necessary to know how many arguments have been passed
  if(argc == 1) {
    printf("citpass requires an argument. Possible arguments are:\n");
    printf("init - Create the file where passwords will be stored, located at $HOME/.cpasswords\n");
    printf("add - Add a password along with associated information to said file\n");
    printf("rm - Remove a password along with associated information from the file\n");
    printf("get - Retrieve a password from the file\n");
  }
  else if(argc == 2){
  // Now, it's necessary to parse command line arguments passed to the program, so
  if (argv[1] == "init") {

  // Initialization
  printf("ass");
  /* Empty file creation */

  /* File encryption */

  }
  else if(argv[1] == 'add') {

  // Addition of password

  /* File unencryption */

  /* Appending an empty entry to the end of the database file */

  /* User now fills the entry with information */

  /* File encryption */

  }
  else if(argv[1] == 'rm') {

  // Removal of password

  /* File unencryption */

  /* Print out list of entries */

  /* User selects entry */

  /* Entry is deleted */

  /* File encryption */

  }
  else if(argv[1] == 'get') {

  // Retrieval of password

  /* File unencryption */

  /* Print out list of entries */

  /* User selects entry */

  /* Password is printed to stdout */

  /* File encryption */
  }
  }
  else if(argc > 2) {
      printf("Too many arguments supplied.\n");
  }
  else {
    printf("Invalid argument. Please provide a valid one.\n");
    printf("Possible arguments are:\n");
    printf("init - Create the file where passwords will be stored, located at $HOME/.cpasswords\n");
    printf("add - Add a password along with associated information to said file\n");
    printf("rm - Remove a password along with associated information from the file\n");
    printf("get - Retrieve a password from the file\n");
  }
  return 0;
}
