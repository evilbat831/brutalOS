/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
*/

#pragma once

// includes
#include <libio/path.h>
#include <libio/handle.h>
#include <libio/reader.h>
#include <libio/writer.h>

namespace IO
{

struct File final :
    public Reader,
    public Writer,
    public Seek,
    public RawHandle
{
private:
    RefPtr<Handle> _handle;
    Optional<IO::Path> _path = NONE;

public:
    const Optional<IO::Path> &path() { return _path; }

    File() {}

    File(const char *path, HjOpenFlag flags = 0);

    File(String path, HjOpenFlag flags = 0);

    File(IO::Path &path, HjOpenFlag flags = 0);

    File(RefPtr<Handle> handle);

    ResultOr<size_t> read(void *buffer, size_t size) override;

    ResultOr<size_t> write(const void *buffer, size_t size) override;

    ResultOr<size_t> call(IOCall call, void *args);

    ResultOr<size_t> seek(SeekFrom from) override;

    ResultOr<size_t> tell() override;

    ResultOr<size_t> length() override;

    ResultOr<HjFileType> type();

    virtual RefPtr<Handle> handle() override { return _handle; }

    bool exist();

    HjResult result()
    {
        if (!_handle)
        {
            return ERR_BAD_HANDLE;
        }

        return _handle->result();
    }
};

} 