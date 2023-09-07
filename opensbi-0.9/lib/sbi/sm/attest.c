#include <sm/attest.h>
#include <sm/gm/SM3.h>
#include <sm/gm/SM2_sv.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_string.h>
#include <sm/print.h>

void hash_domain(struct domain_t *dom)
{
    SM3_STATE hash_ctx;
    unsigned long curr_addr, left_size, counter;
    int hash_granularity = 1 << 20;

    SM3_init(&hash_ctx);

    //hash domain physical memory
    sbi_printf("[%s] Start to hash domain:\n", __func__);
    curr_addr = dom->sbi_domain->measure_addr;
    left_size = dom->sbi_domain->measure_size;
    counter = 0;
    while(left_size > hash_granularity){
        SM3_process(&hash_ctx, (unsigned char*)curr_addr, hash_granularity);
        curr_addr += hash_granularity;
        left_size -= hash_granularity;
        counter++;
        sbi_printf("[%s] hashed %ld MB, left %ld MB\n", __func__, counter, left_size >> 20);
    }
    SM3_process(&hash_ctx, (unsigned char*)curr_addr, (int)left_size);
    sbi_printf("[%s] Finish domain hash, total %ld B\n", __func__, dom->sbi_domain->measure_size);

    SM3_done(&hash_ctx, dom->hash);
}

// initailize Secure Monitor's private key and public key.
void attest_init()
{
    int i;
    struct prikey_t *sm_prikey = (struct prikey_t *)SM_PRI_KEY;
    struct pubkey_t *sm_pubkey = (struct pubkey_t *)SM_PUB_KEY;
    
    i = SM2_Init();
    if(i)
        printm("SM2_Init failed with ret value: %d\n", i);

    i = SM2_KeyGeneration(sm_prikey->dA, sm_pubkey->xA, sm_pubkey->yA);
    if(i)
        printm("SM2_KeyGeneration failed with ret value: %d\n", i);
}

void sign_enclave(void* signature_arg, unsigned char *message, int len)
{
    struct signature_t *signature = (struct signature_t*)signature_arg;
    struct prikey_t *sm_prikey = (struct prikey_t *)SM_PRI_KEY;
    
    SM2_Sign(message, len, sm_prikey->dA, (unsigned char *)(signature->r),
        (unsigned char *)(signature->s));
}

int verify_enclave(void* signature_arg, unsigned char *message, int len)
{
    int ret = 0;
    struct signature_t *signature = (struct signature_t*)signature_arg;
    struct pubkey_t *sm_pubkey = (struct pubkey_t *)SM_PUB_KEY;
    ret = SM2_Verify(message, len, sm_pubkey->xA, sm_pubkey->yA,
        (unsigned char *)(signature->r), (unsigned char *)(signature->s));
    return ret;
}
