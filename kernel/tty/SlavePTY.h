/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <kernel/filesystem/InodeIdentifier.h>
#include <kernel/tty/TTY.h>

namespace Kernel {

class MasterPTY;

class SlavePTY final : public TTY {
public:
    virtual ~SlavePTY() override;

    void on_master_write(const UserOrKernelBuffer&, size_t);
    unsigned index() const { return m_index; }

    time_t time_of_last_write() const { return m_time_of_last_write; }

    virtual FileBlockCondition& block_condition() override;

private:

    virtual String const& tty_name() const override;
    virtual KResultOr<size_t> on_tty_write(const UserOrKernelBuffer&, size_t) override;
    virtual void echo(u8) override;

    virtual bool can_read(const FileDescription&, size_t) const override;
    virtual KResultOr<size_t> read(FileDescription&, u64, UserOrKernelBuffer&, size_t) override;
    virtual bool can_write(const FileDescription&, size_t) const override;
    virtual StringView class_name() const override { return "SlavePTY"; }
    virtual KResult close() override;

    virtual String device_name() const override;

    friend class MasterPTY;
    SlavePTY(MasterPTY&, unsigned index);

    RefPtr<MasterPTY> m_master;
    time_t m_time_of_last_write { 0 };
    unsigned m_index { 0 };
    String m_tty_name;
};

}