/*
 * Ctunnel - Cyptographic tunnel.
 * Copyright (C) 2008-2020 Jess Mahan <ctunnel@alienrobotarmy.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <openssl/opensslv.h>

#include "ctunnel.h"

#ifdef HAVE_OPENSSL

crypto_ctx *openssl_crypto_init(struct options opt, int dir)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    crypto_ctx *ctx = calloc(1, sizeof(crypto_ctx));
#else
    crypto_ctx *ctx = EVP_CIPHER_CTX_new();
#endif

    if (opt.proxy == 0)
    {

        /* STREAM CIPHERS ONLY */
        EVP_CIPHER_CTX_init(ctx);
        EVP_CipherInit_ex(ctx, opt.key.cipher, NULL,
                          opt.key.key, opt.key.iv, dir);
        /* Encrypt final for UDP? */
        EVP_CIPHER_CTX_set_padding(ctx, 1);
    }

    return ctx;
}

void openssl_crypto_reset(crypto_ctx *ctx, struct options opt, int dir)
{
    if (opt.proxy == 0)
    {
        if (dir == 1)
            EVP_EncryptInit_ex(ctx, opt.key.cipher, NULL, opt.key.key, opt.key.iv);
        else
            EVP_DecryptInit_ex(ctx, opt.key.cipher, NULL, opt.key.key, opt.key.iv);
    }
}

void openssl_crypto_deinit(crypto_ctx *ctx)
{
    EVP_CIPHER_CTX_cleanup(ctx);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    free(ctx);
#else
    EVP_CIPHER_CTX_free(ctx);
#endif
}

int openssl_do_encrypt(crypto_ctx *ctx, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        if (!(ret = EVP_EncryptUpdate(ctx, pkt_out->data, &pkt_out->len, pkt->data, pkt->len)))
            ctunnel_log(stderr, LOG_CRIT, "EVP_EncryptUpdate: Error");
    }

    return ret;
}

int openssl_do_decrypt(crypto_ctx *ctx, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        if (!(ret = EVP_DecryptUpdate(ctx, pkt_out->data, &pkt_out->len, pkt->data, pkt->len)))
            ctunnel_log(stderr, LOG_CRIT, "EVP_DecryptUpdate: Error");
    }

    return ret;
}
#else
crypto_ctx gcrypt_crypto_init(struct options opt, int dir)
{
    int ret = 0;

    crypto_ctx ctx;
    ret = gcry_cipher_open(&ctx, opt.key.cipher, opt.key.mode, 0);
    if (!ctx)
    {
        ctunnel_log(stderr, LOG_CRIT, "gcry_open_cipher: %s", gpg_strerror(ret));
        exit(ret);
    }
    ret = gcry_cipher_setkey(ctx, opt.key.key, KEY_MAX);
    if (ret)
    {
        ctunnel_log(stderr, LOG_CRIT, "gcry_cipher_setkey: %s", gpg_strerror(ret));
        exit(ret);
    }
    ret = gcry_cipher_setiv(ctx, opt.key.iv, IV_MAX);
    if (ret)
    {
        ctunnel_log(stderr, LOG_CRIT, "gcry_cipher_setiv: %s", gpg_strerror(ret));
        exit(ret);
    }
    return ctx;
}

void gcrypt_crypto_deinit(crypto_ctx ctx)
{
    gcry_cipher_close(ctx);
}

void gcrypt_crypto_reset(crypto_ctx ctx, struct options opt, int dir)
{
    gcrypt_crypto_deinit(ctx);
    ctx = gcrypt_crypto_init(opt, dir);
}

int gcrypt_do_encrypt(crypto_ctx ctx, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        if (!(ret = gcry_cipher_encrypt(ctx, pkt_out->data, &pkt_out->len, pkt->data, pkt->len)))
	{
            ctunnel_log(stderr, LOG_CRIT, "Encrypt Error: %s", gpg_strerror(ret));
        }
    }

    return ret;
}

int gcrypt_do_decrypt(crypto_ctx ctx, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        if (!(ret = gcry_cipher_decrypt(ctx, pkt_out->data, &pkt_out->len, pkt->data, pkt->len)))
	{
            ctunnel_log(stderr, LOG_CRIT, "Encrypt Error: %s", gpg_strerror(ret));
        }
    }

    return ret;
}
#endif
