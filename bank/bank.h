/*
 * The Bank takes commands from stdin as well as from the ATM.
 *
 * Commands from stdin be handled by bank_process_local_command.
 *
 * Remote commands from the ATM should be handled by
 * bank_process_remote_command.
 *
 * The Bank can read both .card files AND .pin files.
 *
 * Feel free to update the struct and the processing as you desire
 * (though you probably won't need/want to change send/recv).
 */

#ifndef __BANK_H__
#define __BANK_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "hash_table.h"

#define KSIZE 49

typedef struct _User {
  int pin;
  char name[251];
  int balance;
} User;

typedef struct _Bank
{
    // Networking state
    int sockfd;
    struct sockaddr_in rtr_addr;
    struct sockaddr_in bank_addr;

    // Protocol state
    // TODO add more, as needed
    HashTable *users;
    char *key;
} Bank;

Bank* bank_create();
void bank_free(Bank *bank);
ssize_t bank_send(Bank *bank, char *data, size_t data_len);
ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len);
void bank_process_local_command(Bank *bank, char *command, size_t len);
void bank_process_remote_command(Bank *bank, char *command, size_t len);
int do_evp_seal(FILE *rsa_pkey_file, FILE *in_file, FILE *out_file);
int do_evp_unseal(FILE *rsa_pkey_file, FILE *in_file, FILE *out_file);

#endif
