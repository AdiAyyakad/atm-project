#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "crypt.h"

#define SIZE 15
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
    bank->key = malloc(KSIZE);
    memset(bank->key, 0x00, KSIZE);
    fgets(bank->key, KSIZE, fp);

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank->key);
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

void pin_enc(Bank *bank, char *dest, char *src) {
  // do something more secure
  // use bank->key as a salt

  char *enc_ptr = encrypt_src(src, bank->key);
  strcpy(dest, enc_ptr);
  free(enc_ptr);
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

        char hash_content[HASH_PIN_SIZE];
        memset(hash_content, 0x00, HASH_PIN_SIZE);
        pin_enc(bank, hash_content, pin_str);

        fprintf(cardfp, "%s\n", hash_content);
        fclose(cardfp);

        User *user = malloc(sizeof(User));
        strcpy(user->name, username);
        user->pin = atoi(pin_str);
        user->balance = atoi(balance_str);

        char *malloc_username = malloc(strlen(username));
        strncpy(malloc_username, username, strlen(username));

        hash_table_add(bank->users, malloc_username, user);

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
          ((User *) hash_table_find(bank->users, username))->balance = new_balance;
          printf("$%d added to %s\'s account\n", amt, username);
        }
      }

    } else if (strcmp(p, "balance") == 0) {
      char *username = strtok(NULL, " \n");

      if (username == NULL || strlen(username) > 250) {
        printf("Usage: balance <user-name>\n");
      } else {
        User *user = (User *) hash_table_find(bank->users, username);
        if (user == NULL) {
          printf("No such user\n");
        } else {
          printf("$%d\n", user->balance);
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
  memset(sendline, 0x00, 1000);
  command[len]=0;

  char *cmd = decrypt_src(command, bank->key);

  char *p = strtok(cmd, " \n");
  if (p == NULL) return;

  if (strcmp(p, "user-exists") == 0) { // return "yes" or "no"

    char *username = strtok(NULL, " \n");
    User *user = hash_table_find(bank->users, username);
    if (user != NULL) {
      sprintf(sendline, "yes");
    } else {
      sprintf(sendline, "no");
    }

  } else if (strcmp(p, "balance") == 0) { // returns the balance

    // don't need to check for user existence because atm already is locked into one user
    char *user = strtok(NULL, " \n");
    int balance = ((User *) hash_table_find(bank->users, user))->balance;
    sprintf(sendline, "%d", balance);

  } else if (strcmp(p, "withdraw") == 0) {

    char *username = strtok(NULL, " \n");
    char *amt_str = strtok(NULL, " \n");

    User *user = (User *) hash_table_find(bank->users, username);
    int balance = user->balance;
    int new_balance;
    if (amt_str != NULL) new_balance = balance - atoi(amt_str);

    if (amt_str != NULL || new_balance < 0) {
      user->balance = new_balance;
      sprintf(sendline, "success");
    } else {
      sprintf(sendline, "failure");
    }

  } else if (strcmp(p, "pin") == 0) {

    char *username = strtok(NULL, " \n");
    char *pin = strtok(NULL, " \n");

    if (pin_is_valid(pin) == 1) {
      char card_filename[256];
      sprintf(card_filename, "%s.card", username);

      FILE *cardptr = fopen(card_filename, "r");
      char hash_pin[HASH_PIN_SIZE];
      memset(hash_pin, 0x00, HASH_PIN_SIZE);
      fgets(hash_pin, HASH_PIN_SIZE, cardptr);
      hash_pin[strcspn(hash_pin, "\n")] = '\0';

      char hash_pin_input[HASH_PIN_SIZE];
      memset(hash_pin_input, 0x00, HASH_PIN_SIZE);
      pin_enc(bank, hash_pin_input, pin);

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

  char * send = encrypt_src(sendline, bank->key);
  bank_send(bank, send, strlen(sendline)+1);
  free(cmd);
  free(send);
}
