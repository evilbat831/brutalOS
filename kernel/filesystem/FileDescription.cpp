/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/


// includes
#include <base/MemoryStream.h>
#include <kernel/Debug.h>
#include <kernel/devices/BlockDevice.h>
#include <kernel/filesystem/Custody.h>
#include <kernel/filesystem/FIFO.h>
#include <kernel/filesystem/FileDescription.h>
#include <kernel/filesystem/FileSystem.h>
#include <kernel/filesystem/InodeFile.h>
#include <kernel/filesystem/InodeWatcher.h>
#include <kernel/memory/MemoryManager.h>
#include <kernel/net/Socket.h>
#include <kernel/Process.h>
#include <kernel/tty/MasterPTY.h>
#include <kernel/tty/TTY.h>
#include <kernel/UnixTypes.h>
#include <libc/errno_numbers.h>

namespace Kernel {

KResultOr<NonnullRefPtr<FileDescription>> FileDescription::create(Custody& custody)
{
    auto inode_file = InodeFile::create(custody.inode());
    if (inode_file.is_error())
        return inode_file.error();

    auto description = adopt_ref_if_nonnull(new (nothrow) FileDescription(*inode_file.release_value()));
    if (!description)
        return ENOMEM;

    description->m_custody = custody;
    auto result = description->attach();
    if (result.is_error()) {
        dbgln_if(FILEDESCRIPTION_DEBUG, "Failed to create file description for custody: {}", result);
        return result;
    }
    return description.release_nonnull();
}

KResultOr<NonnullRefPtr<FileDescription>> FileDescription::create(File& file)
{
    auto description = adopt_ref_if_nonnull(new (nothrow) FileDescription(file));
    if (!description)
        return ENOMEM;
    auto result = description->attach();
    if (result.is_error()) {
        dbgln_if(FILEDESCRIPTION_DEBUG, "Failed to create file description for file: {}", result);
        return result;
    }
    return description.release_nonnull();
}

FileDescription::FileDescription(File& file)
    : m_file(file)
{
    if (file.is_inode())
        m_inode = static_cast<InodeFile&>(file).inode();

    m_is_directory = metadata().is_directory();
}

FileDescription::~FileDescription()
{
    m_file->detach(*this);
    if (is_fifo())
        static_cast<FIFO*>(m_file.ptr())->detach(m_fifo_direction);

    (void)m_file->close();
    if (m_inode)
        m_inode->detach(*this);

    if (m_inode)
        m_inode->remove_flocks_for_description(*this);
}

KResult FileDescription::attach()
{
    if (m_inode) {
        auto result = m_inode->attach(*this);
        if (result.is_error())
            return result;
    }
    return m_file->attach(*this);
}

Thread::FileBlocker::BlockFlags FileDescription::should_unblock(Thread::FileBlocker::BlockFlags block_flags) const
{
    using BlockFlags = Thread::FileBlocker::BlockFlags;
    BlockFlags unblock_flags = BlockFlags::None;
    if (has_flag(block_flags, BlockFlags::Read) && can_read())
        unblock_flags |= BlockFlags::Read;
    if (has_flag(block_flags, BlockFlags::Write) && can_write())
        unblock_flags |= BlockFlags::Write;


    if (has_any_flag(block_flags, BlockFlags::SocketFlags)) {
        auto* sock = socket();
        VERIFY(sock);
        if (has_flag(block_flags, BlockFlags::Accept) && sock->can_accept())
            unblock_flags |= BlockFlags::Accept;
        if (has_flag(block_flags, BlockFlags::Connect) && sock->setup_state() == Socket::SetupState::Completed)
            unblock_flags |= BlockFlags::Connect;
    }
    return unblock_flags;
}

KResult FileDescription::stat(::stat& buffer)
{
    MutexLocker locker(m_lock);

    if (m_inode)
        return m_inode->metadata().stat(buffer);
    return m_file->stat(buffer);
}

KResultOr<off_t> FileDescription::seek(off_t offset, int whence)
{
    MutexLocker locker(m_lock);
    if (!m_file->is_seekable())
        return ESPIPE;

    off_t new_offset;

    switch (whence) {
    case SEEK_SET:
        new_offset = offset;
        break;
    case SEEK_CUR:
        if (Checked<off_t>::addition_would_overflow(m_current_offset, offset))
            return EOVERFLOW;
        new_offset = m_current_offset + offset;
        break;
    case SEEK_END:
        if (!metadata().is_valid())
            return EIO;
        if (Checked<off_t>::addition_would_overflow(metadata().size, offset))
            return EOVERFLOW;
        new_offset = metadata().size + offset;
        break;
    default:
        return EINVAL;
    }

    if (new_offset < 0)
        return EINVAL;


    m_current_offset = new_offset;

    m_file->did_seek(*this, new_offset);
    if (m_inode)
        m_inode->did_seek(*this, new_offset);
    evaluate_block_conditions();
    return m_current_offset;
}

KResultOr<size_t> FileDescription::read(UserOrKernelBuffer& buffer, u64 offset, size_t count)
{
    if (Checked<u64>::addition_would_overflow(offset, count))
        return EOVERFLOW;
    return m_file->read(*this, offset, buffer, count);
}

KResultOr<size_t> FileDescription::write(u64 offset, UserOrKernelBuffer const& data, size_t data_size)
{
    if (Checked<u64>::addition_would_overflow(offset, data_size))
        return EOVERFLOW;
    return m_file->write(*this, offset, data, data_size);
}

KResultOr<size_t> FileDescription::read(UserOrKernelBuffer& buffer, size_t count)
{
    MutexLocker locker(m_lock);
    if (Checked<off_t>::addition_would_overflow(m_current_offset, count))
        return EOVERFLOW;
    auto nread_or_error = m_file->read(*this, offset(), buffer, count);
    if (!nread_or_error.is_error()) {
        if (m_file->is_seekable())
            m_current_offset += nread_or_error.value();
        evaluate_block_conditions();
    }
    return nread_or_error;
}

KResultOr<size_t> FileDescription::write(const UserOrKernelBuffer& data, size_t size)
{
    MutexLocker locker(m_lock);
    if (Checked<off_t>::addition_would_overflow(m_current_offset, size))
        return EOVERFLOW;
    auto nwritten_or_error = m_file->write(*this, offset(), data, size);
    if (!nwritten_or_error.is_error()) {
        if (m_file->is_seekable())
            m_current_offset += nwritten_or_error.value();
        evaluate_block_conditions();
    }
    return nwritten_or_error;
}

bool FileDescription::can_write() const
{
    return m_file->can_write(*this, offset());
}

bool FileDescription::can_read() const
{
    return m_file->can_read(*this, offset());
}

KResultOr<NonnullOwnPtr<KBuffer>> FileDescription::read_entire_file()
{
    VERIFY(m_file->is_inode());
    VERIFY(m_inode);
    return m_inode->read_entire(this);
}

KResultOr<size_t> FileDescription::get_dir_entries(UserOrKernelBuffer& output_buffer, size_t size)
{
    MutexLocker locker(m_lock, Mutex::Mode::Shared);
    if (!is_directory())
        return ENOTDIR;

    auto metadata = this->metadata();
    if (!metadata.is_valid())
        return EIO;

    size_t remaining = size;
    KResult error = KSuccess;
    u8 stack_buffer[PAGE_SIZE];
    Bytes temp_buffer(stack_buffer, sizeof(stack_buffer));
    OutputMemoryStream stream { temp_buffer };

    auto flush_stream_to_output_buffer = [&error, &stream, &remaining, &output_buffer]() -> bool {
        if (error != 0)
            return false;
        if (stream.size() == 0)
            return true;
        if (remaining < stream.size()) {
            error = EINVAL;
            return false;
        } else if (!output_buffer.write(stream.bytes())) {
            error = EFAULT;
            return false;
        }
        output_buffer = output_buffer.offset(stream.size());
        remaining -= stream.size();
        stream.reset();
        return true;
    };

    KResult result = VirtualFileSystem::the().traverse_directory_inode(*m_inode, [&flush_stream_to_output_buffer, &stream, this](auto& entry) {
        size_t serialized_size = sizeof(ino_t) + sizeof(u8) + sizeof(size_t) + sizeof(char) * entry.name.length();
        if (serialized_size > stream.remaining()) {
            if (!flush_stream_to_output_buffer()) {
                return false;
            }
        }
        stream << (u32)entry.inode.index().value();
        stream << m_inode->fs().internal_file_type_to_directory_entry_type(entry);
        stream << (u32)entry.name.length();
        stream << entry.name.bytes();
        return true;
    });
    flush_stream_to_output_buffer();

    if (result.is_error()) {

        VERIFY(result != -EFAULT);
        return result;
    }

    if (error) {
        return error;
    }
    return size - remaining;
}

bool FileDescription::is_device() const
{
    return m_file->is_device();
}

const Device* FileDescription::device() const
{
    if (!is_device())
        return nullptr;
    return static_cast<const Device*>(m_file.ptr());
}

Device* FileDescription::device()
{
    if (!is_device())
        return nullptr;
    return static_cast<Device*>(m_file.ptr());
}

bool FileDescription::is_tty() const
{
    return m_file->is_tty();
}

const TTY* FileDescription::tty() const
{
    if (!is_tty())
        return nullptr;
    return static_cast<const TTY*>(m_file.ptr());
}

TTY* FileDescription::tty()
{
    if (!is_tty())
        return nullptr;
    return static_cast<TTY*>(m_file.ptr());
}

bool FileDescription::is_inode_watcher() const
{
    return m_file->is_inode_watcher();
}

const InodeWatcher* FileDescription::inode_watcher() const
{
    if (!is_inode_watcher())
        return nullptr;
    return static_cast<const InodeWatcher*>(m_file.ptr());
}

InodeWatcher* FileDescription::inode_watcher()
{
    if (!is_inode_watcher())
        return nullptr;
    return static_cast<InodeWatcher*>(m_file.ptr());
}

bool FileDescription::is_master_pty() const
{
    return m_file->is_master_pty();
}

const MasterPTY* FileDescription::master_pty() const
{
    if (!is_master_pty())
        return nullptr;
    return static_cast<const MasterPTY*>(m_file.ptr());
}

MasterPTY* FileDescription::master_pty()
{
    if (!is_master_pty())
        return nullptr;
    return static_cast<MasterPTY*>(m_file.ptr());
}

KResult FileDescription::close()
{
    if (m_file->attach_count() > 0)
        return KSuccess;
    return m_file->close();
}

String FileDescription::absolute_path() const
{
    if (m_custody)
        return m_custody->absolute_path();
    return m_file->absolute_path(*this);
}

InodeMetadata FileDescription::metadata() const
{
    if (m_inode)
        return m_inode->metadata();
    return {};
}

KResultOr<Memory::Region*> FileDescription::mmap(Process& process, Memory::VirtualRange const& range, u64 offset, int prot, bool shared)
{
    MutexLocker locker(m_lock);
    return m_file->mmap(process, *this, range, offset, prot, shared);
}

KResult FileDescription::truncate(u64 length)
{
    MutexLocker locker(m_lock);
    return m_file->truncate(length);
}

bool FileDescription::is_fifo() const
{
    return m_file->is_fifo();
}

FIFO* FileDescription::fifo()
{
    if (!is_fifo())
        return nullptr;
    return static_cast<FIFO*>(m_file.ptr());
}

bool FileDescription::is_socket() const
{
    return m_file->is_socket();
}

Socket* FileDescription::socket()
{
    if (!is_socket())
        return nullptr;
    return static_cast<Socket*>(m_file.ptr());
}

const Socket* FileDescription::socket() const
{
    if (!is_socket())
        return nullptr;
    return static_cast<const Socket*>(m_file.ptr());
}

void FileDescription::set_file_flags(u32 flags)
{
    MutexLocker locker(m_lock);
    m_is_blocking = !(flags & O_NONBLOCK);
    m_should_append = flags & O_APPEND;
    m_direct = flags & O_DIRECT;
    m_file_flags = flags;
}

KResult FileDescription::chmod(mode_t mode)
{
    MutexLocker locker(m_lock);
    return m_file->chmod(*this, mode);
}

KResult FileDescription::chown(uid_t uid, gid_t gid)
{
    MutexLocker locker(m_lock);
    return m_file->chown(*this, uid, gid);
}

FileBlockCondition& FileDescription::block_condition()
{
    return m_file->block_condition();
}

KResult FileDescription::apply_flock(Process const& process, Userspace<flock const*> lock)
{
    if (!m_inode)
        return EBADF;

    return m_inode->apply_flock(process, *this, lock);
}

KResult FileDescription::get_flock(Userspace<flock*> lock) const
{
    if (!m_inode)
        return EBADF;

    return m_inode->get_flock(*this, lock);
}
}