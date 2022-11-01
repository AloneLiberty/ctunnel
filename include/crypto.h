/* crypto.c */
crypto_ctx gcrypt_crypto_init(struct options opt, int dir);
void gcrypt_crypto_reset(crypto_ctx ctx, struct options opt, int dir);
void gcrypt_crypto_deinit(crypto_ctx ctx);
int gcrypt_do_encrypt(crypto_ctx ctx, struct Packet *pkt, struct Packet *pkt_out);
int gcrypt_do_decrypt(crypto_ctx ctx, struct Packet *pkt, struct Packet *pkt_out);
/* crypto.c */
crypto_ctx *openssl_crypto_init(struct options opt, int dir);
void openssl_crypto_reset(crypto_ctx *ctx, struct options opt, int dir);
void openssl_crypto_deinit(crypto_ctx *ctx);
int openssl_do_encrypt(crypto_ctx *ctx, struct Packet *pkt, struct Packet *pkt_out);
int openssl_do_decrypt(crypto_ctx *ctx, struct Packet *pkt, struct Packet *pkt_out);
