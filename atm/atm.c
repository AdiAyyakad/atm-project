#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 10000

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

void contact(ATM *atm, char *message, char *response) {
  atm_send(atm, message, strlen(message)+1);
  int n = atm_recv(atm, response, SIZE);
  response[n] = 0;
}

/**
  * Returns current user balance
  */
int get_balance(ATM *atm) {
  char response[SIZE], message[259];
  memset(response, 0x00, SIZE);
  memset(message, 0x00, 259);
  sprintf(message, "balance %s", atm->current_user);

  contact(atm, message, response);

  return atoi(response);
}

/**
  * Returns 0 for user does exist, anything else for does not exist
  */
int user_exists(ATM *atm, char *username) {
  // check if user exists from bank
  char response[SIZE], message[263];
  memset(response, 0x00, SIZE);
  memset(message, 0x00, 263);
  sprintf(message, "user-exists %s", username);

  contact(atm, message, response);

  return strcmp(response, "yes");
}

// return 0 if success, otherwise if failed
int check_pin(ATM *atm, char *username, char *pin) {
  char response[SIZE], message[260];
  memset(response, 0x00, SIZE);
  memset(message, 0x00, 260);
  sprintf(message, "pin %s %s", username, pin);

  contact(atm, message, response);

  return strcmp(response, "success");
}

void atm_process_command(ATM *atm, char *command)
{
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

      if (user_exists(atm, username) != 0) {
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
      fclose(cardfp);

      char ipin_str[5];
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

        if (check_pin(atm, username, ipin_str) == 0) {
          printf("Authorized\n");
          strncpy(atm->current_user, username, strlen(username));
        } else {
          printf("Not authorized\n");
        }
      }

    } else if (strcmp(p, "withdraw") == 0) {
      p = strtok(NULL, " \n");
      int amt;
      if (p != NULL) amt = atoi(p);

      if (strlen(atm->current_user) == 0) {
        printf("No user logged in\n");
      } else if (p == NULL || amt < 0) {
        printf("Usage: withdraw <amt>\n");
      } else {
        char response[SIZE], msg[271]; // 9 + 250 + 1 + 10 + 1
        sprintf(msg, "withdraw %s %d", atm->current_user, amt);
        atm_send(atm, msg, strlen(msg)+1);
        int n = atm_recv(atm, response, SIZE);
        response[n] = 0;

        if (strcmp(response, "success") == 0) {
          printf("$%d dispensed\n", amt);
        } else {
          printf("Insufficient funds\n");
        }
      }

    } else if (strcmp(p, "balance") == 0) {
      p = strtok(NULL, " \n");

      if (strlen(atm->current_user) == 0) {
        printf("No user logged in\n");
      } else if (p != NULL) {
        printf("Usage: balance\n");
      } else {
        int balance = get_balance(atm);
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
      printf("Invalid command\n");
    }
}
