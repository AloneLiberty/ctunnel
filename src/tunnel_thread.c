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

#include <pthread.h>

#include "ctunnel.h"

void *ctunnel_mainloop(void *arg)
{
	struct Ctunnel *ct;
	struct Network *net_srv;
	struct Network *net_cli;
	struct Packet pkt;
	fd_set rfd;
	int ret = 0, dir = 0;
	time_t t, tl;
	//extern int threads[MAX_THREADS];
	struct timeval tv;
#ifdef HAVE_OPENSSL
	do_encrypt = openssl_do_encrypt;
	do_decrypt = openssl_do_decrypt;
	crypto_init = openssl_crypto_init;
	crypto_deinit = openssl_crypto_deinit;
#else
	do_encrypt = gcrypt_do_encrypt;
	do_decrypt = gcrypt_do_decrypt;
	crypto_init = gcrypt_crypto_init;
	crypto_deinit = gcrypt_crypto_deinit;
#endif
        struct xfer_stats rx_st, tx_st;

        t = tl = time(NULL);
	ct = (struct Ctunnel *)arg;

	if (ct->opt.proto == TCP)
	{
		net_srv = calloc(1, sizeof(struct Network));
		net_cli = calloc(1, sizeof(struct Network));
		net_cli->sockfd = (int)ct->clisockfd;
		net_srv->sockfd = (int)ct->srvsockfd;
	}
	else
	{
		net_srv = (struct Network *)ct->net_srv;
		net_cli = (struct Network *)ct->net_cli;
	}

	if (ct->opt.proxy == 0)
	{
		ct->ectx = crypto_init(ct->opt, 1);
		ct->dctx = crypto_init(ct->opt, 0);
	}

	if (ct->opt.comp == 1)
		z_compress_init(&ct->opt, &ct->comp);

        pkt.size = ct->opt.packet_size * 2;
	pkt.data = malloc(sizeof(unsigned char) * pkt.size);

        xfer_stats_init(&tx_st, (int)t);
        xfer_stats_init(&rx_st, (int)t);
        //if (ct->opt.stats == 1)
        //        xfer_stats_print(stdout, &tx_st, &rx_st);

	while (1)
	{
		if (ct->opt.proto == UDP && ct->opt.proxy == 0)
		{
			crypto_deinit(ct->ectx);
			crypto_deinit(ct->dctx);
			ct->ectx = crypto_init(ct->opt, 1);
			ct->dctx = crypto_init(ct->opt, 0);
		}

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		FD_ZERO(&rfd);
		FD_SET(net_srv->sockfd, &rfd);
		FD_SET(net_cli->sockfd, &rfd);
		ret = select(net_srv->sockfd + net_cli->sockfd + 1, &rfd,
					 NULL, NULL, &tv);
		if (ret < 0)
			break;

		t = time(NULL);

		//if (ct->opt.stats == 1)
                //        xfer_stats_print(stdout, &tx_st, &rx_st);

		if (FD_ISSET(net_srv->sockfd, &rfd))
		{
			if (ct->opt.role == ROLE_CLIENT)
				dir = 0;
			if (ct->opt.role == ROLE_SERVER)
				dir = 1;

			ret = net_read(ct, net_srv, &pkt, dir);
			if (ret < 0)
				break;

			if (pkt.len > 0)
                        {
			        if (ct->opt.role == ROLE_CLIENT)
				        dir = 1;
			        if (ct->opt.role == ROLE_SERVER)
				        dir = 0;

			        ret = net_write(ct, net_cli, &pkt, dir);
			        if (ret <= 0) 
				        break;
                        }
		}
		if (FD_ISSET(net_cli->sockfd, &rfd))
		{
			if (ct->opt.role == ROLE_CLIENT)
				dir = 1;
			if (ct->opt.role == ROLE_SERVER)
				dir = 0;

			ret = net_read(ct, net_cli, &pkt, dir);
			if (ret < 0)
				break;

			if (ct->opt.role == ROLE_CLIENT)
				dir = 0;
			if (ct->opt.role == ROLE_SERVER)
				dir = 1;

			if (pkt.len > 0)
                        {
			        ret = net_write(ct, net_srv, &pkt, dir);
			        if (ret <= 0)
				        break;
                        }
		}
		if (ct->opt.proto == UDP && ct->opt.comp == 1)
		{
			z_compress_reset(&ct->comp);
		}
	}

        if (ct->opt.stats == 1)
        {
		t = time(NULL);
		if (ct->opt.role == 0)
                {
		        xfer_stats_update(&tx_st, net_cli->rate.tx.total, t);
			xfer_stats_update(&rx_st, net_cli->rate.rx.total, t);
                }
		if (ct->opt.role == 1)
                {
			xfer_stats_update(&rx_st, net_srv->rate.rx.total, t);
			xfer_stats_update(&tx_st, net_srv->rate.tx.total, t);
                }
                xfer_stats_print(stdout, &tx_st, &rx_st);
        }

	/* Connection closed */
	FD_CLR(net_cli->sockfd, &rfd);
	FD_CLR(net_srv->sockfd, &rfd);

	if (ct->opt.proto == TCP)
	{
		pthread_mutex_lock(&mutex);
		threads[ct->id] = 2;
		pthread_mutex_unlock(&mutex);
		ctunnel_log(stderr, LOG_INFO, "THREAD: Exit %d", ct->id);
		net_close(net_cli);
		net_close(net_srv);
		if (ct->opt.proxy == 0)
		{
			crypto_deinit(ct->ectx);
			crypto_deinit(ct->dctx);
		}
		if (ct->opt.comp == 1)
		{
			z_compress_end(&ct->comp);
		}

		free(net_cli);
		free(net_srv);
	}
	//free(ct);
	free(pkt.data);

	if (ct->opt.proto == TCP)
		pthread_exit(0);
	return NULL;
}
