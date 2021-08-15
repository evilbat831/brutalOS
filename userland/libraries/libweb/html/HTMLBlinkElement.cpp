/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <libcore/Timer.h>
#include <libweb/css/StyleValue.h>
#include <libweb/html/HTMLBlinkElement.h>

namespace Web::HTML {

HTMLBlinkElement::HTMLBlinkElement(DOM::Document& document, QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
    , m_timer(Core::Timer::construct())
{
    m_timer->set_interval(500);
    m_timer->on_timeout = [this] { blink(); };
    m_timer->start();
}

HTMLBlinkElement::~HTMLBlinkElement()
{
}

void HTMLBlinkElement::blink()
{
    if (!layout_node())
        return;

    layout_node()->set_visible(!layout_node()->is_visible());
    layout_node()->set_needs_display();
}

}