/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <base/Function.h>
#include <libjs/runtime/Error.h>
#include <libjs/runtime/ErrorPrototype.h>
#include <libjs/runtime/GlobalObject.h>
#include <libjs/runtime/PrimitiveString.h>
#include <libjs/runtime/Value.h>

namespace JS {

ErrorPrototype::ErrorPrototype(GlobalObject& global_object)
    : Object(*global_object.object_prototype())
{
}

void ErrorPrototype::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    Object::initialize(global_object);
    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_direct_property(vm.names.name, js_string(vm, "Error"), attr);
    define_direct_property(vm.names.message, js_string(vm, ""), attr);
    define_native_function(vm.names.toString, to_string, 0, attr);
}

JS_DEFINE_NATIVE_FUNCTION(ErrorPrototype::to_string)
{
    auto this_value = vm.this_value(global_object);
    if (!this_value.is_object()) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotAnObject, this_value.to_string_without_side_effects());
        return {};
    }
    auto& this_object = this_value.as_object();

    String name = "Error";
    auto name_property = this_object.get(vm.names.name);
    if (vm.exception())
        return {};
    if (!name_property.is_undefined()) {
        name = name_property.to_string(global_object);
        if (vm.exception())
            return {};
    }

    String message = "";
    auto message_property = this_object.get(vm.names.message);
    if (vm.exception())
        return {};
    if (!message_property.is_undefined()) {
        message = message_property.to_string(global_object);
        if (vm.exception())
            return {};
    }

    if (name.is_empty())
        return js_string(vm, message);
    if (message.is_empty())
        return js_string(vm, name);
    return js_string(vm, String::formatted("{}: {}", name, message));
}

#define __JS_ENUMERATE(ClassName, snake_name, PrototypeName, ConstructorName, ArrayType) \
    PrototypeName::PrototypeName(GlobalObject& global_object)                            \
        : Object(*global_object.error_prototype())                                       \
    {                                                                                    \
    }                                                                                    \
                                                                                         \
    void PrototypeName::initialize(GlobalObject& global_object)                          \
    {                                                                                    \
        auto& vm = this->vm();                                                           \
        Object::initialize(global_object);                                               \
        u8 attr = Attribute::Writable | Attribute::Configurable;                         \
        define_direct_property(vm.names.name, js_string(vm, #ClassName), attr);          \
        define_direct_property(vm.names.message, js_string(vm, ""), attr);               \
    }

JS_ENUMERATE_NATIVE_ERRORS
#undef __JS_ENUMERATE

}