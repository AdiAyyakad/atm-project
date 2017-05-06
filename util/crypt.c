#include "crypt.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void xor_strings(char *dest, char *src, char *key) {
  int i, ksize = strlen(key);
  for (i = 0; i < strlen(src); i++)
    dest[i] = src[i] ^ key[i % ksize];
}

// Mutates src to be the encrypted data
char* encrypt_src(char *src, char *key) {
  // encrypt src
  char *encrypted = malloc(strlen(src) + 1);
  memset(encrypted, 0x00, strlen(src) + 1);

  xor_strings(encrypted, src, key);

  return encrypted;
}

// Mutates src to be the decrypted data
char* decrypt_src(char *src, char *key) {
  // decrypt src
  char *decrypted = malloc(strlen(src) + 1);
  memset(decrypted, 0x00, strlen(src) + 1);

  xor_strings(decrypted, src, key);

  return decrypted;
}
