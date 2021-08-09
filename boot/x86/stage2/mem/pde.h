/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef _BOOT_X86_STAGE2_MEM_PDE_H
#define _BOOT_X86_STAGE2_MEM_PDE_H

// includes
#include "../types.h"

#define table_desc_t uint32_t
#define pde_t uint32_t

enum TABLE_DESC_PAGE_FLAGS {
    TABLE_DESC_PRESENT = 0,
    TABLE_DESC_WRITABLE,
    TABLE_DESC_USER,
    TABLE_DESC_PWT,
    TABLE_DESC_PCD,
    TABLE_DESC_ACCESSED,
    TABLE_DESC_DIRTY,
    TABLE_DESC_4MB,
    TABLE_DESC_CPU_GLOBAL,
    TABLE_DESC_LV4_GLOBAL,
    TABLE_DESC_COPY_ON_WRITE,
    TABLE_DESC_ZEROING_ON_DEMAND,
    TABLE_DESC_FRAME = 12
};

#endif 