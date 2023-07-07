#ifndef _ATTEST_H
#define _ATTEST_H

#include "sm/enclave.h"
#include "sm/domain.h"

void attest_init();

void hash_enclave(struct enclave_t* enclave, void* hash, uintptr_t nonce);

void update_enclave_hash(char *output, void* hash, uintptr_t nonce_arg);

void hash_domain(struct domain_t* dom);

void sign_enclave(void* signature, unsigned char *message, int len);

int verify_enclave(void* signature, unsigned char *message, int len);

#endif /* _ATTEST_H */
