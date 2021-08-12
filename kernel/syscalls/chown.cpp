/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

// includes
#include <kernel/filesystem/FileDescription.h>
#include <kernel/Process.h>

namespace Kernel {

KResultOr<FlatPtr> Process::sys$fchown(int fd, uid_t uid, gid_t gid)
{
    VERIFY_PROCESS_BIG_LOCK_ACQUIRED(this);
    REQUIRE_PROMISE(chown);
    auto description = fds().file_description(fd);
    if (!description)
        return EBADF;
    return description->chown(uid, gid);
}

KResultOr<FlatPtr> Process::sys$chown(Userspace<const Syscall::SC_chown_params*> user_params)
{
    VERIFY_PROCESS_BIG_LOCK_ACQUIRED(this);
    REQUIRE_PROMISE(chown);
    Syscall::SC_chown_params params;
    if (!copy_from_user(&params, user_params))
        return EFAULT;
    auto path = get_syscall_path_argument(params.path);
    if (path.is_error())
        return path.error();
    return VirtualFileSystem::the().chown(path.value()->view(), params.uid, params.gid, current_directory());
}

}