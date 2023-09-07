#ifndef _ATTEST_H
#define _ATTEST_H

#include "sm/domain.h"

void attest_init();

void hash_domain(struct domain_t* dom);

void sign_enclave(void* signature, unsigned char *message, int len);

int verify_enclave(void* signature, unsigned char *message, int len);

#endif /* _ATTEST_H */
