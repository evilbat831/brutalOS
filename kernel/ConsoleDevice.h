/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <base/CircularQueue.h>
#include <base/Vector.h>
#include <kernel/devices/CharacterDevice.h>

namespace Kernel {

class ConsoleDevice final : public CharacterDevice {
    BASE_MAKE_ETERNAL
public:
    static ConsoleDevice& the();
    static void initialize();
    static bool is_initialized();

    ConsoleDevice();
    virtual ~ConsoleDevice() override;


    virtual bool can_read(const Kernel::FileDescription&, size_t) const override;
    virtual bool can_write(const Kernel::FileDescription&, size_t) const override { return true; }
    virtual Kernel::KResultOr<size_t> read(FileDescription&, u64, Kernel::UserOrKernelBuffer&, size_t) override;
    virtual Kernel::KResultOr<size_t> write(FileDescription&, u64, const Kernel::UserOrKernelBuffer&, size_t) override;
    virtual StringView class_name() const override { return "Console"; }

    void put_char(char);

    const CircularQueue<char, 16384>& logbuffer() const { return m_logbuffer; }

    virtual mode_t required_mode() const override { return 0666; }
    virtual String device_name() const override { return "console"; }

private:
    CircularQueue<char, 16384> m_logbuffer;
};

}