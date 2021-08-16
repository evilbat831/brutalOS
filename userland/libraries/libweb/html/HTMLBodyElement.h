/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libweb/html/HTMLElement.h>

namespace Web::HTML {

class HTMLBodyElement final : public HTMLElement {
public:
    using WrapperType = Bindings::HTMLBodyElementWrapper;

    HTMLBodyElement(DOM::Document&, QualifiedName);
    virtual ~HTMLBodyElement() override;

    virtual void parse_attribute(const FlyString&, const String&) override;
    virtual void apply_presentational_hints(CSS::StyleProperties&) const override;

private:
    // ^HTML::GlobalEventHandlers
    virtual EventTarget& global_event_handlers_to_event_target() override;

    RefPtr<CSS::ImageStyleValue> m_background_style_value;
};

}
