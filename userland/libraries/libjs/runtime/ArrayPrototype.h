/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libjs/runtime/Array.h>

namespace JS {

class ArrayPrototype final : public Array {
    JS_OBJECT(ArrayPrototype, Array);

public:
    ArrayPrototype(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~ArrayPrototype() override;

private:
    JS_DECLARE_NATIVE_FUNCTION(filter);
    JS_DECLARE_NATIVE_FUNCTION(for_each);
    JS_DECLARE_NATIVE_FUNCTION(map);
    JS_DECLARE_NATIVE_FUNCTION(pop);
    JS_DECLARE_NATIVE_FUNCTION(push);
    JS_DECLARE_NATIVE_FUNCTION(shift);
    JS_DECLARE_NATIVE_FUNCTION(to_string);
    JS_DECLARE_NATIVE_FUNCTION(to_locale_string);
    JS_DECLARE_NATIVE_FUNCTION(unshift);
    JS_DECLARE_NATIVE_FUNCTION(join);
    JS_DECLARE_NATIVE_FUNCTION(concat);
    JS_DECLARE_NATIVE_FUNCTION(slice);
    JS_DECLARE_NATIVE_FUNCTION(index_of);
    JS_DECLARE_NATIVE_FUNCTION(reduce);
    JS_DECLARE_NATIVE_FUNCTION(reduce_right);
    JS_DECLARE_NATIVE_FUNCTION(reverse);
    JS_DECLARE_NATIVE_FUNCTION(sort);
    JS_DECLARE_NATIVE_FUNCTION(last_index_of);
    JS_DECLARE_NATIVE_FUNCTION(includes);
    JS_DECLARE_NATIVE_FUNCTION(find);
    JS_DECLARE_NATIVE_FUNCTION(find_index);
    JS_DECLARE_NATIVE_FUNCTION(find_last);
    JS_DECLARE_NATIVE_FUNCTION(find_last_index);
    JS_DECLARE_NATIVE_FUNCTION(some);
    JS_DECLARE_NATIVE_FUNCTION(every);
    JS_DECLARE_NATIVE_FUNCTION(splice);
    JS_DECLARE_NATIVE_FUNCTION(fill);
    JS_DECLARE_NATIVE_FUNCTION(values);
    JS_DECLARE_NATIVE_FUNCTION(flat);
    JS_DECLARE_NATIVE_FUNCTION(flat_map);
    JS_DECLARE_NATIVE_FUNCTION(at);
    JS_DECLARE_NATIVE_FUNCTION(keys);
    JS_DECLARE_NATIVE_FUNCTION(entries);
    JS_DECLARE_NATIVE_FUNCTION(copy_within);
};

}
