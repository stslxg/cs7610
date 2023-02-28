#ifndef PACK_H
#define PACK_H

uint32_t unpack_type(unsigned char *buf);

size_t pack_up(unsigned char *buf, uint32_t sender);
UpMessage unpack_up(unsigned char *buf);

size_t pack_vc(unsigned char *buf, uint32_t server_id, uint32_t attempted);
View_Change unpack_vc(unsigned char *buf);

size_t pack_vp(unsigned char *buf, uint32_t server_id, uint32_t installed);
VC_Proof unpack_vp(unsigned char *buf);

#endif
