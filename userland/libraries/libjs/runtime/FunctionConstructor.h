/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libjs/runtime/NativeFunction.h>

namespace JS {

class FunctionConstructor final : public NativeFunction {
    JS_OBJECT(FunctionConstructor, NativeFunction);

public:
    static RefPtr<FunctionExpression> create_dynamic_function_node(GlobalObject& global_object, FunctionObject& new_target, FunctionKind kind);

    explicit FunctionConstructor(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~FunctionConstructor() override;

    virtual Value call() override;
    virtual Value construct(FunctionObject& new_target) override;

private:
    virtual bool has_constructor() const override { return true; }
};

}