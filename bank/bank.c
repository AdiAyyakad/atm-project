#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
    // TODO set up more, as needed

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
    // TODO: Implement the bank's local commands
    char * p = strtok(command, " \n");

    if (strcmp(p, "create-user") == 0) {
      char *username, *pin_str, *balance_str;
      int pin, balance;

      p = strtok(NULL, " \n");
      username = p;
      p = strtok(NULL, " \n");
      pin_str = p;
      p = strtok(NULL, " \n");
      balance_str = p;

      pin = atoi(pin_str);
      balance = atoi(balance_str);

      if (username == NULL || strlen(username) > 250 || pin_str == NULL || balance_str == NULL || pin < 0 || (pin - 9999) > 0 || balance < 0) {
        printf("Usage: create-user <user-name> <pin> <balance>\n");
      } else {
        printf("Success: create-user user-name: %s, pin: %d, balance: %d\n", username, pin, balance);
        char card_filename[255];
        memset(card_filename, '\0', 255);
        strcat(card_filename, username);
        strcat(card_filename, ".card");

        FILE *cardfp = fopen(card_filename, "w+");
        if (cardfp == NULL) {
          printf("Error creating card file for user %s\n", username);
        }

        char *content = username;
        fprintf(cardfp, "%s\n", content);

        fclose(cardfp);

        printf("Created user %s\n", username);
      }
    } else if (strcmp(p, "deposit") == 0) {
      char *username, *amt_str;
      int amt;

      p = strtok(NULL, " \n");
      username = p;
      p = strtok(NULL, " \n");
      amt_str = p;

      amt = atoi(amt_str);

      if (username == NULL || strlen(username) > 250 || amt_str == NULL || amt < 0) {
        printf("Usage: deposit <user-name> <amt>\n");
      } else {
        printf("Success: deposit\n");
      }

    } else if (strcmp(p, "balance") == 0) {
      char *username;

      p = strtok(NULL, " \n");
      username = p;

      if (username == NULL || strlen(username) > 250) {
        printf("Usage: balance <user-name>\n");
      } else {
        printf("Success: balance\n");
      }
    } else {
      printf("Invalid command\n");
    }
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes
	 * it back to the ATM before printing it to stdout.
	 */

	/*
    char sendline[1000];
    command[len]=0;
    sprintf(sendline, "Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
	*/
}
