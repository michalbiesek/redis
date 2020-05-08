/* pmem.c - Persistent Memory interface
 *
 * Copyright (c) 2020, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must start the above copyright notice,
 *     this quicklist of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this quicklist of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "server.h"

#include <math.h>
#include <stdio.h>

#define THRESHOLD_STEP_NORMAL 0.05
#define THRESHOLD_STEP_AGGRESIVE (THRESHOLD_STEP_NORMAL*5)
#define THRESHOLD_UP(val, step)  ((size_t)ceil((1+(step))*val))
#define THRESHOLD_DOWN(val, step) ((size_t)floor((1-(step))*val))

static inline size_t absDiff(size_t a, size_t b) {
    return a > b ? (a - b) : (b - a);
}

/* Initialize the pmem threshold. */
void pmemThresholdInit(void)
{
    switch(server.memory_alloc_policy) {
        case MEM_POLICY_ONLY_DRAM:
            zmalloc_set_threshold(UINT_MAX);
            break;
        case MEM_POLICY_ONLY_PMEM:
            zmalloc_set_threshold(0U);
            break;
        case MEM_POLICY_THRESHOLD:
            zmalloc_set_threshold(server.static_threshold);
            break;
        case MEM_POLICY_RATIO:
            zmalloc_set_threshold(server.initial_dynamic_threshold);
            break;
        default:
            serverAssert(NULL);
    }
}

void adjustPmemThresholdCycle(void) {
    if (server.memory_alloc_policy == MEM_POLICY_RATIO) {
        run_with_period(server.ratio_check_period) {
            /* Difference between target ratio and current ratio in last checkpoint*/
            static double ratio_diff_checkpoint;
            /* PMEM and DRAM utilization in last checkpoint*/
            static size_t total_memory_checkpoint;
            size_t pmem_memory = zmalloc_used_pmem_memory();
            size_t dram_memory = zmalloc_used_memory();
            size_t total_memory_current = pmem_memory + dram_memory;
            // do not modify threshold when change in memory usage is too small
            if (absDiff(total_memory_checkpoint, total_memory_current) > 100) {
                double current_ratio = (double)pmem_memory/dram_memory;
                double current_ratio_diff = fabs(current_ratio - server.target_pmem_dram_ratio);
                if (current_ratio_diff > 0.02) {
                    //current ratio is worse than moment before
                    double variableMultipler = current_ratio/server.target_pmem_dram_ratio;
                    double step = (current_ratio_diff < ratio_diff_checkpoint) ?
                                  variableMultipler*THRESHOLD_STEP_NORMAL : variableMultipler*THRESHOLD_STEP_AGGRESIVE;
                    size_t threshold = zmalloc_get_threshold();
                    if (server.target_pmem_dram_ratio < current_ratio) {
                        size_t higher_threshold = THRESHOLD_UP(threshold,step);
                        if (higher_threshold <= server.dynamic_threshold_max)
                            zmalloc_set_threshold(higher_threshold);
                    } else {
                        size_t lower_threshold = THRESHOLD_DOWN(threshold,step);
                        if (lower_threshold >= server.dynamic_threshold_min)
                            zmalloc_set_threshold(lower_threshold);
                    }
                }
                ratio_diff_checkpoint = current_ratio_diff;
            }
            total_memory_checkpoint = total_memory_current;
        }
    }
}

#if defined(USE_MEMKIND)
#include <libpmemlog.h>
static int
process_chunk(const void *buf, size_t len, void *arg) {
    int* fd = (int*)arg;
    if (write(*fd, buf, len) >= 0) {
        return 0;
    }else {
        return 1;
    }
}

int pmemLogWrite(void* log, const void* data, size_t len) {
    return pmemlog_append(log, data, len);
}

void pmemLogDeInit(void* log) {
    pmemlog_close(log);
}

void pmemLogReset(void* log) {
    pmemlog_rewind(log);
}

size_t pmemLogCurrentSize(void* log) {
    return (size_t)pmemlog_tell(log);
}

void pmemCopyLog(void* src, int fd) {
    pmemlog_walk(src, 0, process_chunk, (void*)&fd);
}

int pmemLogcheck(const char* path) {
    return pmemlog_check(path);
}

void* pmemLogInit(const char* path, size_t size) {
    return pmemlog_create(path, size, 0666);
}

void* pmemLogOpen(const char* path) {
    return pmemlog_open(path);
}

ssize_t pmemLogWriteToAof(void* log, const void* data, size_t len, int fd) {
    size_t current_size = pmemLogCurrentSize(log);
    if (current_size + len < pmemLogMaxSize(log)) {
        if(!pmemLogWrite(log, data, len)) {
            return 0;
        } else {
            return -1;
        }
     } else {
        pmemCopyLog(log, fd);
        pmemLogReset(log);
        if(!pmemLogWrite(log, data, len)) {
            return (ssize_t)current_size;
        } else {
            return -1;
        }
     }
}

size_t pmemLogMaxSize(void* log) {
    return pmemlog_nbyte(log);
}
#else

ssize_t pmemLogWriteToAof(void* log, const void* data, size_t len, int fd) {
    (void)log;
    (void)data;
    (void)len;
    (void)fd;
    return 0;
}

int pmemLogWrite(void* log, const void* data, size_t len) {
    (void)log;
    (void)data;
    (void)len;
    return 0;
}

void pmemLogDeInit(void* log) {
    (void)log;
}

void pmemLogReset(void* log) {
    (void)log;
}

void pmemCopyLog(void* src, int fd) {
    (void)src;
    (void)fd;
}

size_t pmemLogCurrentSize(void* log) {
    (void)log;
    return 0;
}

size_t pmemLogMaxSize(void* log) {
    (void)log;
    return 0;
}

int pmemLogcheck(const char* path) {
    (void)path;
    return 0;
}

void* pmemLogOpen(const char* path) {
    (void)path;
    return NULL;
}

void* pmemLogInit(const char* path, size_t size) {
    (void)path;
    (void)size;
    return NULL;
}
#endif

