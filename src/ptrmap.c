#include "ptrmap.h"
#include "common.h"


uint8_t *map_meta(ptrmap m)
{
	return (uint8_t *) SEXTEND(m._packed & BITRANGE(48, 4), 16);
}

key_t *pmap_buf(ptrmap m)
{
	return (void **) SEXTEND(m._packed & BITRANGE(48, 4), 16);
}

len_t pmap_len(ptrmap m)
{
	return (1 << (m._packed >> 58)) - 1;
}


key_t *pmap_find(ptrmap *m, key_t *k, hash_func *fn)
{
}

key_t *pmap_push(ptrmap *m, key_t *k, hash_func *fn)
{
}

key_t *pmap_reserve(ptrmap *m, len_t n)
{
}

