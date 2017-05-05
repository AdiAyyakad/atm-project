#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <openssl/sha.h>

#define SIZE 1
#define HASH_PIN_SIZE 261

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

    memset(bank->salt, 0x00, SNPSIZE);
    memset(bank->pepper, 0x00, SNPSIZE);

    fgets(bank->salt, SNPSIZE, fp);
    fgets(bank->pepper, SNPSIZE, fp);

    bank->salt[strcspn(bank->salt, "\n")] = '\0';
    bank->pepper[strcspn(bank->pepper, "\n")] = '\0';

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);

        hash_table_free(bank->users);
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

// Return 1 if valid, 0 if invalid
int username_is_valid(char *username){
    if (username == NULL || strlen(username) > 250) return 0;

    int i;
    for(i = 0; i < strlen(username); i++) {
        if ((username[i] < 'A') || // if character is less than 'A', it is invalid
            (username[i] > 'Z' && username[i] < 'a') || // if it is between 'Z' and 'a', it is invalid
            (username[i] > 'z')) // if it is greater than 'z', it is invalid
          return 0;
    }

    return 1;
}

int pin_is_valid(char *pin) {
  if (pin == NULL || strlen(pin) != 4) return 0;

  int i;
  for (i = 0; i < strlen(pin); i++) {
    if (pin[i] < '0' || pin[1] > '9')
      return 0;
  }

  return 1;
}

int balance_is_valid(char *balance) {
  if (balance == NULL || strlen(balance) > 10) return 0; // max int has 10 digits
  int b = atoi(balance);
  if (b < 0 || b > INT_MAX) return 0;

  int i;
  for (i = 0; i < strlen(balance); i++) {
    if (balance[i] < '0' || balance[i] > '9')
      return 0;
  }

  return 1;
}

void pin_hash(Bank *bank, char *dest, char *pin) {
  unsigned char hash_content[HASH_PIN_SIZE];
  memset(hash_content, 0x00, HASH_PIN_SIZE);
  snprintf(hash_content, HASH_PIN_SIZE, "%s%s%s", bank->salt, pin, bank->pepper);

  SHA1(hash_content, HASH_PIN_SIZE, hash_content);
  strcpy(dest, src);
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    char * p = strtok(command, " \n");

    if (p == NULL) return;

    if (strcmp(p, "create-user") == 0) {
      char *username = strtok(NULL, " \n");
      char *pin_str = strtok(NULL, " \n");
      char *balance_str = strtok(NULL, " \n");

      if (username_is_valid(username) != 1 || pin_is_valid(pin_str) != 1 || balance_is_valid(balance_str) != 1) {
        printf("Usage: create-user <user-name> <pin> <balance>\n");
      } else {
        char card_filename[256];
        sprintf(card_filename, "%s.card", username);

        FILE *cardfp = fopen(card_filename, "w+");
        if (cardfp == NULL) {
          printf("Error creating card file for user %s\n", username);
        }

        char hash_content[SHA_DIGEST_LENGTH];
        pin_hash(bank, hash_content, pin_str);

        fprintf(cardfp, "%s\n", hash_content);
        fclose(cardfp);

        int *balancep = malloc(sizeof(int));
        *balancep = atoi(balance_str);

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

      char *userptr = malloc(strlen(user));
      strcpy(userptr, user);
      hash_table_add(bank->users, userptr, nbp);
      sprintf(sendline, "success");
    } else {
      sprintf(sendline, "failure");
    }

  } else if (strcmp(p, "pin") == 0) {

    char *user = strtok(NULL, " \n");
    char *pin = strtok(NULL, " \n");

    if (pin_is_valid(pin) == 1) {
      char card_filename[256];
      sprintf(card_filename, "%s.card", user);

      FILE *cardptr = fopen(card_filename, "r");
      char hash_pin[SHA_DIGEST_LENGTH];
      memset(hash_pin, 0x00, SHA_DIGEST_LENGTH);
      fgets(hash_pin, SHA_DIGEST_LENGTH, cardptr);
      hash_pin[strcspn(hash_pin, "\n")] = '\0';

      unsigned char hash_pin_input[SHA_DIGEST_LENGTH];
      memset(hash_pin_input, 0x00, SHA_DIGEST_LENGTH);
      pin_hash(bank, hash_pin_input, pin);

      if (strcmp(hash_pin, hash_pin_input) == 0)
        sprintf(sendline, "success");
      else
        sprintf(sendline, "failure");

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
