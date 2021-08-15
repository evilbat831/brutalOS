/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <libweb/css/parser/Parser.h>
#include <libweb/css/SelectorEngine.h>
#include <libweb/dom/ParentNode.h>
#include <libweb/Dump.h>

namespace Web::DOM {

RefPtr<Element> ParentNode::query_selector(const StringView& selector_text)
{
    auto maybe_selectors = parse_selector(CSS::ParsingContext(*this), selector_text);
    if (!maybe_selectors.has_value())
        return {};

    auto selectors = maybe_selectors.value();

    for (auto& selector : selectors)
        dump_selector(selector);

    RefPtr<Element> result;
    for_each_in_inclusive_subtree_of_type<Element>([&](auto& element) {
        for (auto& selector : selectors) {
            if (SelectorEngine::matches(selector, element)) {
                result = element;
                return IterationDecision::Break;
            }
        }
        return IterationDecision::Continue;
    });

    return result;
}

NonnullRefPtrVector<Element> ParentNode::query_selector_all(const StringView& selector_text)
{
    auto maybe_selectors = parse_selector(CSS::ParsingContext(*this), selector_text);
    if (!maybe_selectors.has_value())
        return {};

    auto selectors = maybe_selectors.value();

    for (auto& selector : selectors)
        dump_selector(selector);

    NonnullRefPtrVector<Element> elements;
    for_each_in_inclusive_subtree_of_type<Element>([&](auto& element) {
        for (auto& selector : selectors) {
            if (SelectorEngine::matches(selector, element)) {
                elements.append(element);
            }
        }
        return IterationDecision::Continue;
    });

    return elements;
}

RefPtr<Element> ParentNode::first_element_child()
{
    return first_child_of_type<Element>();
}

RefPtr<Element> ParentNode::last_element_child()
{
    return last_child_of_type<Element>();
}

u32 ParentNode::child_element_count() const
{
    u32 count = 0;
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (is<Element>(child))
            ++count;
    }
    return count;
}

}