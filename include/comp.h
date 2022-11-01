/* comp.c */
void z_compress_init(struct options *opt, struct Compress *comp);
int z_compress(z_stream *str, struct Packet *pkt, struct Packet *pkt_out);
int z_uncompress(z_stream *str, struct Packet *pkt, struct Packet *pkt_out);
void z_compress_reset(struct Compress *comp);
void z_compress_end(struct Compress *comp);
