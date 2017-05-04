#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

ATM* atm_create()
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    // TODO set up more, as needed
    memset(atm->current_user, '\0', 251);

    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol
    char *p = strtok(command, " \n");
    if (p == NULL) return;

    if (strcmp(p, "begin-session") == 0) {
      // check if someone is already logged in
      if (strlen(atm->current_user) != 0) {
        printf("A user is already logged in\n");
        return;
      }

      char *username = strtok(NULL, " \n");

      if (username == NULL || strlen(username) > 250) {
        printf("Usage: begin-session <user-name>\n");
        return;
      }

      // check if user exists from bank
      char response[10000], message[262];
      sprintf(message, "user-exists %s", username);
      atm_send(atm, message, strlen(message));
      int n = atm_recv(atm, response, 10000);
      response[n]=0;
      if (strcmp(response, "yes") != 0){
          printf("No such user\n");
          return;
      }

      char card_filename[256];
      sprintf(card_filename, "%s.card", username);

      FILE *cardfp = fopen(card_filename, "r");
      if (cardfp == NULL) {
        printf("Unable to access %s\'s card\n", username);
        return;
      }

      char ipin_str[5], card_pin[129];
      printf("PIN? ");

      if (fgets(ipin_str, sizeof(ipin_str), stdin)) {
        p = strchr(ipin_str, '\n');
        if (p) {
          *p = '\0';
        } else {
          int ch;
          // newline not found, flush to end of line
          while (((ch = getchar()) != '\n') && !feof(stdin) && !ferror(stdin));
        }

        if (fgets(card_pin, sizeof(card_pin), cardfp)) {
          p = strchr(card_pin, '\n');
          if (p) {
            *p = '\0';
          } else {
            int ch;
            while ((ch = getchar()) != '\n' && !feof(cardfp) && !ferror(cardfp));
          }
          fclose(cardfp);

          card_pin[4] = '\0';
          if (strcmp(ipin_str, card_pin) == 0) {
            printf("Authorized\n");
            strncpy(atm->current_user, username, strlen(username));
          } else {
            printf("Not authorized\n");
          }
        }
      }

    } else if (strcmp(p, "withdraw") == 0) {
      p = strtok(NULL, " \n");
      int amt = atoi(p);

      if (strlen(atm->current_user) == 0) {
        printf("No user logged in\n");
      } else if (p == NULL || amt < 0) {
        printf("Usage: withdraw <amt>\n");
      } else {
        int current_balance = 1000; // TODO: get current balance
        if (current_balance - amt < 0) {
          printf("Insufficient funds\n");
        } else {
          printf("$%d dispensed\n", amt);
          // TODO: update balance to be current_balance - amt
        }
      }

    } else if (strcmp(p, "balance") == 0) {
      p = strtok(NULL, " \n");

      if (strlen(atm->current_user) == 0) {
        printf("No user logged in\n");
      } else if (p != NULL) {
        printf("Usage: balance\n");
      } else {
        int balance = 10; // TODO: get balance
        printf("$%d\n", balance);
      }

    } else if (strcmp(p, "end-session") == 0) {
      if (strlen(atm->current_user) == 0) {
        printf("No user logged in\n");
      } else {
        strcpy(atm->current_user, "");
        printf("User logged out\n");
      }
    } else {
      printf("Invalid command %s\n", p);
    }

	/*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */

	/*
    char recvline[10000];
    int n;

    atm_send(atm, command, strlen(command));
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    fputs(recvline,stdout);
	*/
}
