/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <libweb/html/HTMLElement.h>

namespace Web::HTML {

class HTMLTableCellElement final : public HTMLElement {
public:
    using WrapperType = Bindings::HTMLTableCellElementWrapper;

    HTMLTableCellElement(DOM::Document&, QualifiedName);
    virtual ~HTMLTableCellElement() override;

private:
    virtual void apply_presentational_hints(CSS::StyleProperties&) const override;
};

}
