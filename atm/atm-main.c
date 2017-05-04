/*
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char user_input[1000];

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
      printf("Error opening ATM initialization file\n");
      return 64;
    }

    ATM *atm = atm_create();

    printf("ATM: ");

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);
        char username[252] = "";
        if (strlen(atm->current_user) > 0) sprintf(username, " (%s)", atm->current_user);
        printf("ATM%s: ", username);
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
