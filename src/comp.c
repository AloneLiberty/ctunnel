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

#include "ctunnel.h"

void z_compress_init(struct options *opt, struct Compress *comp)
{
    int ret = 0;

    comp->deflate.zalloc = Z_NULL;
    comp->deflate.zfree = Z_NULL;
    comp->deflate.opaque = Z_NULL;
    ret = deflateInit(&comp->deflate, opt->comp_level);
    if (ret != Z_OK)
    {
        ctunnel_log(stderr, LOG_CRIT, "Error %d Initializing Zlib (deflate)", ret);
        exit(1);
    }

    comp->inflate.zalloc = Z_NULL;
    comp->inflate.zfree = Z_NULL;
    comp->inflate.opaque = Z_NULL;
    comp->inflate.avail_in = 0;
    comp->inflate.next_in = Z_NULL;
    ret = inflateInit(&comp->inflate);
    if (ret != Z_OK)
    {
        ctunnel_log(stderr, LOG_CRIT, "Error %d Initializing Zlib (inflate)", ret);
        exit(1);
    }
}

void z_compress_reset(struct Compress *comp)
{
    deflateReset(&comp->deflate);
    inflateReset(&comp->inflate);
}

void z_compress_end(struct Compress *comp)
{
    deflateEnd(&comp->deflate);
    inflateEnd(&comp->inflate);
}

int z_compress(z_stream *str, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        str->avail_in = pkt->len;
        str->next_in = pkt->data;
        do
        {
            str->avail_out = pkt_out->size - pkt_out->len;
            str->next_out = pkt_out->data + pkt_out->len;
            if (str->avail_in > 0)
            {
                //fprintf(stderr, "deflate %d\n", str->avail_in);
                ret = deflate(str, Z_SYNC_FLUSH);
                //fprintf(stderr, "deflate done %d size %d avail %d\n", ret, pkt_out->size - str->avail_out);
                if (ret != Z_OK)
                {
                    ctunnel_log(stderr, LOG_CRIT, "Error %d deflating data", ret);
                    exit(1);
                }
                ret = pkt_out->len = pkt_out->size - str->avail_out;
                if (str->avail_out == 0)
                {
                    pkt_out->size *= 2;
                    pkt_out->data = realloc(pkt_out->data, sizeof(unsigned char) * pkt_out->size);
                }
            }
        } while (str->avail_out == 0);
        if (str->avail_in > 0)
        {
            ctunnel_log(stderr, LOG_CRIT, "Short deflate %d remaining", str->avail_in);
            exit(1);
        }
    }

    return ret;
}

int z_uncompress(z_stream *str, struct Packet *pkt, struct Packet *pkt_out)
{
    int ret = 0;

    pkt_out->size = pkt->size;
    pkt_out->data = malloc(sizeof(unsigned char) * pkt_out->size);
    pkt_out->len = 0;

    if (pkt->len > 0)
    {
        str->avail_in = pkt->len;
        str->next_in = pkt->data;
        do
        {
            str->avail_out = pkt_out->size - pkt_out->len;
            str->next_out = pkt_out->data + pkt_out->len;
            if (str->avail_in > 0)
            {
                //fprintf(stderr, "inflate %d\n", str->avail_in);
                ret = inflate(str, Z_SYNC_FLUSH);
                //fprintf(stderr, "inflate done %d size %d avail %d\n", ret, str->avail_in, pkt_out->size - str->avail_out);
                if (ret != Z_OK)
                {
                    ctunnel_log(stderr, LOG_WARNING, "Error %d inflating data len %d size in %d size out %d avail in %d avail out %d", ret, pkt->len, pkt->size, pkt_out->size, str->avail_in, str->avail_out);
                    return ret ;
                }
                pkt_out->len = pkt_out->size - str->avail_out;
                if (str->avail_out == 0)
                {
                    pkt_out->size *= 2;
                    pkt_out->data = realloc(pkt_out->data, sizeof(unsigned char) * pkt_out->size);
                }
            }
        } while (str->avail_out == 0);
        if (str->avail_in > 0)
        {
	    ctunnel_log(stderr, LOG_CRIT, "Short inflate %d remaining", str->avail_in);
            exit(1);
        }
    }

    return ret;
}
