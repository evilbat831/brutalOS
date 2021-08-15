/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libjs/runtime/NativeFunction.h>

namespace JS {

class ObjectConstructor final : public NativeFunction {
    JS_OBJECT(ObjectConstructor, NativeFunction);

public:
    explicit ObjectConstructor(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~ObjectConstructor() override;

    virtual Value call() override;
    virtual Value construct(FunctionObject& new_target) override;

private:
    virtual bool has_constructor() const override { return true; }

    JS_DECLARE_NATIVE_FUNCTION(define_property);
    JS_DECLARE_NATIVE_FUNCTION(define_properties);
    JS_DECLARE_NATIVE_FUNCTION(is);
    JS_DECLARE_NATIVE_FUNCTION(get_own_property_descriptor);
    JS_DECLARE_NATIVE_FUNCTION(get_own_property_descriptors);
    JS_DECLARE_NATIVE_FUNCTION(get_own_property_names);
    JS_DECLARE_NATIVE_FUNCTION(get_own_property_symbols);
    JS_DECLARE_NATIVE_FUNCTION(get_prototype_of);
    JS_DECLARE_NATIVE_FUNCTION(set_prototype_of);
    JS_DECLARE_NATIVE_FUNCTION(is_extensible);
    JS_DECLARE_NATIVE_FUNCTION(is_frozen);
    JS_DECLARE_NATIVE_FUNCTION(is_sealed);
    JS_DECLARE_NATIVE_FUNCTION(prevent_extensions);
    JS_DECLARE_NATIVE_FUNCTION(seal);
    JS_DECLARE_NATIVE_FUNCTION(freeze);
    JS_DECLARE_NATIVE_FUNCTION(from_entries);
    JS_DECLARE_NATIVE_FUNCTION(keys);
    JS_DECLARE_NATIVE_FUNCTION(values);
    JS_DECLARE_NATIVE_FUNCTION(entries);
    JS_DECLARE_NATIVE_FUNCTION(create);
    JS_DECLARE_NATIVE_FUNCTION(has_own);
    JS_DECLARE_NATIVE_FUNCTION(assign);
};

}