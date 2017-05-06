#ifndef __CRYPTH__
#define __CRYPTH__

#define KSIZE 49

// xors strings together
void xor_strings(char *dest, char *src, char *key);
// Returns a malloced char* to the encrypted data
char * encrypt_src(char *src, char *key);
// Returns a malloced char* to be the decrypted data
char * decrypt_src(char *src, char *key);

#endif
