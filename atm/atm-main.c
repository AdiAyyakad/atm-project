/*
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char prompt[256] = "ATM: ";

int main(int argc, char **argv)
{
    char user_input[1000];

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
      printf("Error opening ATM initialization file\n");
      return 64;
    }

    ATM *atm = atm_create();

    printf("%s", prompt);

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);
        char username[252] = "";
        if (strlen(atm->current_user) > 0) sprintf(username, " %s", atm->current_user);
        sprintf(prompt, "ATM%s: ", username);
        printf("%s", prompt);
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
