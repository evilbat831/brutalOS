/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

// includes
#include <kernel/filesystem/AnonymousFile.h>
#include <kernel/memory/AnonymousVMObject.h>
#include <kernel/Process.h>

namespace Kernel {

AnonymousFile::AnonymousFile(NonnullRefPtr<Memory::AnonymousVMObject> vmobject)
    : m_vmobject(move(vmobject))
{
}

AnonymousFile::~AnonymousFile()
{
}

KResultOr<Memory::Region*> AnonymousFile::mmap(Process& process, FileDescription&, Memory::VirtualRange const& range, u64 offset, int prot, bool shared)
{
    if (offset != 0)
        return EINVAL;

    if (range.size() != m_vmobject->size())
        return EINVAL;

    return process.address_space().allocate_region_with_vmobject(range, m_vmobject, offset, {}, prot, shared);
}

}