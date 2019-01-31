#include "server.h"
#include "atomicvar.h"

#include <libpmem.h>
#include "memkind.h"

#define update_memkind_malloc_stat_alloc(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    atomicIncr(used_memory,__n); \
} while(0)

#define update_memkind_malloc_stat_free(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    atomicDecr(used_memory,__n); \
} while(0)

static size_t used_memory = 0;

size_t memkind_malloc_usable_size_wrapper(void *ptr) {
    return memkind_malloc_usable_size(server.pmem_kind1 ,ptr);
}

void *memkind_alloc_wrapper(size_t size) {
    void *ptr = memkind_malloc(server.pmem_kind1, size);
    if (ptr) {
        size =  memkind_malloc_usable_size(server.pmem_kind1, ptr);
        update_memkind_malloc_stat_alloc(size);
    }
    return ptr;
}

void *memkind_calloc_wrapper(size_t size) {
    void *ptr = memkind_calloc(server.pmem_kind1, 1, size);
    if (ptr) {
        size =  memkind_malloc_usable_size(server.pmem_kind1, ptr);
        update_memkind_malloc_stat_alloc(size);
    }
    return ptr;
}

void *memkind_realloc_wrapper(void *ptr, size_t size) {
    size_t oldsize;
    if (ptr == NULL) return memkind_alloc_wrapper(size);
    oldsize = memkind_malloc_usable_size(server.pmem_kind1 ,ptr);
    void *newptr = memkind_realloc(server.pmem_kind1, ptr, size);
    if (newptr) {
        update_memkind_malloc_stat_free(oldsize);
        size = memkind_malloc_usable_size(server.pmem_kind1, newptr);
        update_memkind_malloc_stat_alloc(size);
    }
    return newptr;
}
int memkind_is_pmem_wrapper(void *ptr) {
    return memkind_is_pmem(server.pmem_kind1 ,ptr);
}

void memkind_free_wrapper(void *ptr) {
    if(!ptr) return;
    memkind_is_pmem_wrapper(ptr);
    size_t oldsize = memkind_malloc_usable_size(server.pmem_kind1 ,ptr);
    update_memkind_malloc_stat_free(oldsize);
    memkind_free(server.pmem_kind1, ptr);
}

size_t memkind_malloc_used_memory(void){
    size_t um;
    atomicGet(used_memory,um);
    return um;
}

void *pmem_memcpy_wrapper(void *pmemdest, const void *src, size_t len){
    return pmem_memcpy(pmemdest, src, len, PMEM_F_MEM_NONTEMPORAL|PMEM_F_MEM_NODRAIN);
}

void *pmem_memset_wrapper(void *pmemdest, int c, size_t len){
    return pmem_memset(pmemdest, c, len, PMEM_F_MEM_NONTEMPORAL|PMEM_F_MEM_NODRAIN);
}

