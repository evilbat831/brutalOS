/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// incldues
#include <libjs/runtime/Object.h>
#include <libweb/Forward.h>

namespace Web {
namespace Bindings {

class LocationObject final : public JS::Object {
    JS_OBJECT(LocationObject, JS::Object);

public:
    explicit LocationObject(JS::GlobalObject&);
    virtual void initialize(JS::GlobalObject&) override;
    virtual ~LocationObject() override;

private:
    JS_DECLARE_NATIVE_FUNCTION(reload);

    JS_DECLARE_NATIVE_FUNCTION(href_getter);
    JS_DECLARE_NATIVE_FUNCTION(href_setter);

    JS_DECLARE_NATIVE_FUNCTION(host_getter);
    JS_DECLARE_NATIVE_FUNCTION(hostname_getter);
    JS_DECLARE_NATIVE_FUNCTION(pathname_getter);
    JS_DECLARE_NATIVE_FUNCTION(hash_getter);
    JS_DECLARE_NATIVE_FUNCTION(search_getter);
    JS_DECLARE_NATIVE_FUNCTION(protocol_getter);
};

}
}
