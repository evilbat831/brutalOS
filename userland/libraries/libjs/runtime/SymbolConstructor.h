/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libjs/runtime/NativeFunction.h>

namespace JS {

class SymbolConstructor final : public NativeFunction {
    JS_OBJECT(SymbolConstructor, NativeFunction);

public:
    explicit SymbolConstructor(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~SymbolConstructor() override;

    virtual Value call() override;
    virtual Value construct(FunctionObject& new_target) override;

private:
    virtual bool has_constructor() const override { return true; }

    JS_DECLARE_NATIVE_FUNCTION(for_);
    JS_DECLARE_NATIVE_FUNCTION(key_for);
};

}
