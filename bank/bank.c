#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 1

Bank* bank_create(FILE *fp)
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    bank->users = hash_table_create(SIZE);

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    char * p = strtok(command, " \n");

    if (p == NULL) return;

    if (strcmp(p, "create-user") == 0) {
      char *username = strtok(NULL, " \n");
      char *pin_str = strtok(NULL, " \n");
      char *balance_str = strtok(NULL, " \n");
      int balance, pin;
      if (balance_str != NULL) balance = atoi(balance_str);
      if (pin_str != NULL) pin = atoi(pin_str);

      if (username == NULL || strlen(username) > 250 || pin_str == NULL || balance_str == NULL || strlen(pin_str) != 4 || pin < 0 || balance < 0) {
        printf("Usage: create-user <user-name> <pin> <balance>\n");
      } else {
        char card_filename[256];
        sprintf(card_filename, "%s.card", username);

        FILE *cardfp = fopen(card_filename, "w+");
        if (cardfp == NULL) {
          printf("Error creating card file for user %s\n", username);
        }

        char *content = pin_str;
        fprintf(cardfp, "%s\n", content);
        fclose(cardfp);

        int *balancep = malloc(sizeof(int));
        *balancep = balance;

        char *malloc_username = malloc(strlen(username));
        strncpy(malloc_username, username, strlen(username));

        hash_table_add(bank->users, malloc_username, balancep);

        printf("Created user %s\n", username);
      }
    } else if (strcmp(p, "deposit") == 0) {
      char *username = strtok(NULL, " \n");
      char *amt_str = strtok(NULL, " \n");
      int amt;
      if (amt_str != NULL) amt = atoi(amt_str);

      if (username == NULL || strlen(username) > 250 || amt_str == NULL || amt < 0) {
        printf("Usage: deposit <user-name> <amt>\n");
      } else if (hash_table_find(bank->users, username) == NULL) {
        printf("No such user\n");
      } else {
        int new_balance = amt + *((int *) hash_table_find(bank->users, username));

        if (new_balance < 0) {
          printf("Too rich for this program\n");
        } else {
          int *nbp = malloc(sizeof(int));
          *nbp = new_balance;
          hash_table_add(bank->users, username, nbp);
          printf("$%d added to %s\'s account\n", amt, username);
        }
      }

    } else if (strcmp(p, "balance") == 0) {
      char *username = strtok(NULL, " \n");

      if (username == NULL || strlen(username) > 250) {
        printf("Usage: balance <user-name>\n");
      } else {
        int *balance = (int *) hash_table_find(bank->users, username);
        if (balance == NULL) {
          printf("No such user %d\n", hash_table_size(bank->users));
        } else {
          printf("$%d\n", *balance);
        }
      }
    } else {
      printf("Invalid command\n");
    }

    fflush(stdout);
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
  char sendline[1000];
  command[len]=0;

  char *p = strtok(command, " \n");
  if (p == NULL) return;

  if (strcmp(p, "user-exists") == 0) { // return "yes" or "no"

    char *user = strtok(NULL, " \n");
    if (hash_table_find(bank->users, user) != NULL) {
      sprintf(sendline, "yes");
    } else {
      sprintf(sendline, "no");
    }

  } else if (strcmp(p, "balance") == 0) { // returns the balance

    // don't need to check for user existence because atm already is locked into one user
    char *user = strtok(NULL, " \n");
    int *balancep = (int *) hash_table_find(bank->users, user);
    sprintf(sendline, "%d", *balancep);

  } else if (strcmp(p, "withdraw") == 0) {

    char *user = strtok(NULL, " \n");
    char *amt_str = strtok(NULL, " \n");
    int balance = *((int *) hash_table_find(bank->users, user));

    if (amt_str != NULL) {
      int new_balance = balance - atoi(amt_str);
      int *nbp = malloc(sizeof(int));
      *nbp = new_balance;
      hash_table_add(bank->users, user, nbp);
      sprintf(sendline, "success");
    } else {
      sprintf(sendline, "failure");
    }

  } else {

    // should never happen
    sprintf(sendline, "Invalid command");
    printf("Invalid command\n");

  }

  bank_send(bank, sendline, strlen(sendline)+1);
}
