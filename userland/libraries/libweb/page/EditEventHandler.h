/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <base/Types.h>
#include <libweb/Forward.h>

namespace Web {

class EditEventHandler {
public:
    explicit EditEventHandler(BrowsingContext& frame)
        : m_frame(frame)
    {
    }

    virtual ~EditEventHandler() = default;

    virtual void handle_delete_character_after(const DOM::Position&);
    virtual void handle_delete(DOM::Range&);
    virtual void handle_insert(DOM::Position, u32 code_point);

private:
    BrowsingContext& m_frame;
};

}
