/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>

#include "allocator_perf_tool/TimerSysTime.hpp"
#include "common.h"
#include "memkind.h"

class MemkindDefaultKindTests: public :: testing::Test
{

protected:
    void SetUp()
    {}

    void TearDown()
    {}
};

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultMallocZero)
{
    void *test1 = nullptr;

    test1 = memkind_malloc(MEMKIND_DEFAULT, 0);
    ASSERT_EQ(test1, nullptr);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultMallocSizeMax)
{
    void *test1 = nullptr;

    errno = 0;
    test1 = memkind_malloc(MEMKIND_DEFAULT, SIZE_MAX);
    ASSERT_EQ(test1, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

/*
 * Test will check if it is not possible to allocate memory
 * with calloc arguments size or num equal to zero
 */
TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultCallocZero)
{
    void *test = nullptr;
    size_t size = 10;
    size_t num = 10;

    test = memkind_calloc(MEMKIND_DEFAULT, 0, num);
    ASSERT_EQ(test, nullptr);

    test = memkind_calloc(MEMKIND_DEFAULT, size, 0);
    ASSERT_EQ(test, nullptr);

    test = memkind_calloc(MEMKIND_DEFAULT, 0, 0);
    ASSERT_EQ(test, nullptr);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultCallocSizeMax)
{
    void *test = nullptr;
    size_t size = SIZE_MAX;
    size_t num = 1;
    errno = 0;

    test = memkind_calloc(MEMKIND_DEFAULT, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultCallocNumMax)
{
    void *test = nullptr;
    size_t size = 10;
    size_t num = SIZE_MAX;
    errno = 0;

    test = memkind_calloc(MEMKIND_DEFAULT, size, num);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, ENOMEM);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocZero)
{
    size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test, nullptr);

    new_test = memkind_realloc(MEMKIND_DEFAULT, test, 0);
    ASSERT_EQ(new_test, nullptr);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocSizeMax)
{
    size_t size = 1 * KB;
    void *test = nullptr;
    void *new_test = nullptr;

    test = memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test, nullptr);
    errno = 0;
    new_test = memkind_realloc(MEMKIND_DEFAULT, test, SIZE_MAX);
    ASSERT_EQ(new_test, nullptr);
    ASSERT_EQ(errno, ENOMEM);

    memkind_free(MEMKIND_DEFAULT, test);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocNullptr)
{
    size_t size = 1 * KB;
    void *test = nullptr;

    test = memkind_realloc(MEMKIND_DEFAULT, test, size);
    ASSERT_NE(test, nullptr);

    memkind_free(MEMKIND_DEFAULT, test);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocNullptrSizeMax)
{
    void *test = nullptr;

    test = memkind_realloc(MEMKIND_DEFAULT, test, SIZE_MAX);
    ASSERT_EQ(test, nullptr);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocNullptrZero)
{
    void *test = nullptr;

    test = memkind_realloc(MEMKIND_DEFAULT, test, 0);
    ASSERT_EQ(test, nullptr);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocIncreaseSize)
{
    size_t size = 1 * KB;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] = "test_TC_MEMKIND_DefaultReallocIncreaseSize";
    int status;

    test1 = (char *)memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test1, nullptr);

    sprintf(test1, "%s", val);

    size *= 2;
    test2 = (char *)memkind_realloc(MEMKIND_DEFAULT, test1, size);
    ASSERT_NE(test2, nullptr);
    status = memcmp(val, test2, sizeof(val));
    ASSERT_EQ(status, 0);

    memkind_free(MEMKIND_DEFAULT, test2);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultReallocDecreaseSize)
{
    size_t size = 1 * KB;
    char *test1 = nullptr;
    char *test2 = nullptr;
    const char val[] = "test_TC_MEMKIND_DefaultReallocDecreaseSize";
    int status;

    test1 = (char *)memkind_malloc(MEMKIND_DEFAULT, size);
    ASSERT_NE(test1, nullptr);

    sprintf(test1, "%s", val);

    size = 4;
    test2 = (char *)memkind_realloc(MEMKIND_DEFAULT, test1, size);
    ASSERT_NE(test2, nullptr);
    status = memcmp(val, test2, size);
    ASSERT_EQ(status, 0);

    memkind_free(MEMKIND_DEFAULT, test2);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultFreeNullptr)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        memkind_free(MEMKIND_DEFAULT, nullptr);
    } while(timer.getElapsedTime() < test_time);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultFreeNullptrAlloc)
{
    const double test_time = 5;

    TimerSysTime timer;
    timer.start();
    do {
        void *ptr = memkind_malloc(MEMKIND_DEFAULT, 512);
        memkind_free(MEMKIND_DEFAULT, ptr);
        ptr = memkind_malloc(MEMKIND_DEFAULT, 512);
        memkind_free(nullptr, ptr);
    } while(timer.getElapsedTime() < test_time);
}

TEST_F(MemkindDefaultKindTests,
       test_TC_MEMKIND_DefaultPosixMemalignWrongAlignmentLessThanVoidAndNotPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = 3;
    int ret;
    errno = 0;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, wrong_alignment, size);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, 0);
}

TEST_F(MemkindDefaultKindTests,
       test_TC_MEMKIND_DefaultPosixMemalignWrongAlignmentLessThanVoidAndPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = sizeof(void *)/2;
    int ret;
    errno = 0;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, wrong_alignment, size);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, 0);
}

TEST_F(MemkindDefaultKindTests,
       test_TC_MEMKIND_DefaultPosixMemalignWrongAlignmentNotPowerOfTwo)
{
    void *test = nullptr;
    size_t size = 32;
    size_t wrong_alignment = sizeof(void *)+1;
    int ret;
    errno = 0;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, wrong_alignment, size);
    ASSERT_EQ(ret, EINVAL);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, 0);
}

TEST_F(MemkindDefaultKindTests,
       test_TC_MEMKIND_DefaultPosixMemalignLowestCorrectAlignment)
{
    void *test = nullptr;
    size_t size = 32;
    size_t alignment = sizeof(void *);
    int ret;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, alignment, size);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(test, nullptr);

    memkind_free(MEMKIND_DEFAULT, test);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultPosixMemalignSizeZero)
{
    void *test = nullptr;
    size_t alignment = sizeof(void *);
    int ret;
    errno = 0;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, alignment, 0);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, 0);
}

TEST_F(MemkindDefaultKindTests, test_TC_MEMKIND_DefaultPosixMemalignSizeMax)
{
    void *test = nullptr;
    size_t alignment = 64;
    int ret;
    errno = 0;

    ret = memkind_posix_memalign(MEMKIND_DEFAULT, &test, alignment, SIZE_MAX);
    ASSERT_EQ(ret, ENOMEM);
    ASSERT_EQ(test, nullptr);
    ASSERT_EQ(errno, 0);
}
