#include "crypt.h"
#include <string.h>
#include <stdlib.h>

// Mutates src to be the encrypted data
char* encrypt_src(char *src, char *key) {
  // encrypt src
  char *encrypted = malloc(strlen(src) + 1);
  memset(encrypted, 0x00, strlen(src) + 1);
  strcpy(encrypted, src);

  return encrypted;
}

// Mutates src to be the decrypted data
char* decrypt_src(char *src, char *key) {
  // decrypt src
  char *decrypted = malloc(strlen(src) + 1);
  memset(decrypted, 0x00, strlen(src) + 1);
  strcpy(decrypted, src);

  return decrypted;
}
