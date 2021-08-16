/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include <libweb/css/parser/Parser.h>
#include <libweb/dom/ElementFactory.h>
#include <libweb/dom/HTMLCollection.h>
#include <libweb/html/HTMLTableColElement.h>
#include <libweb/html/HTMLTableElement.h>
#include <libweb/html/HTMLTableRowElement.h>
#include <libweb/Namespace.h>

namespace Web::HTML {

HTMLTableElement::HTMLTableElement(DOM::Document& document, QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
{
}

HTMLTableElement::~HTMLTableElement()
{
}

void HTMLTableElement::apply_presentational_hints(CSS::StyleProperties& style) const
{
    for_each_attribute([&](auto& name, auto& value) {
        if (name == HTML::AttributeNames::width) {
            if (auto parsed_value = parse_html_length(document(), value))
                style.set_property(CSS::PropertyID::Width, parsed_value.release_nonnull());
            return;
        }
        if (name == HTML::AttributeNames::height) {
            if (auto parsed_value = parse_html_length(document(), value))
                style.set_property(CSS::PropertyID::Height, parsed_value.release_nonnull());
            return;
        }
        if (name == HTML::AttributeNames::bgcolor) {
            auto color = Color::from_string(value);
            if (color.has_value())
                style.set_property(CSS::PropertyID::BackgroundColor, CSS::ColorStyleValue::create(color.value()));
            return;
        }
    });
}

RefPtr<HTMLTableCaptionElement> HTMLTableElement::caption()
{
    return first_child_of_type<HTMLTableCaptionElement>();
}

void HTMLTableElement::set_caption(HTMLTableCaptionElement* caption)
{
    VERIFY(caption);

    delete_caption();

    pre_insert(*caption, first_child());
}

NonnullRefPtr<HTMLTableCaptionElement> HTMLTableElement::create_caption()
{
    auto maybe_caption = caption();
    if (maybe_caption) {
        return *maybe_caption;
    }

    auto caption = DOM::create_element(document(), TagNames::caption, Namespace::HTML);
    pre_insert(caption, first_child());
    return caption;
}

void HTMLTableElement::delete_caption()
{
    auto maybe_caption = caption();
    if (maybe_caption) {
        maybe_caption->remove(false);
    }
}

RefPtr<HTMLTableSectionElement> HTMLTableElement::t_head()
{
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::thead)
                return table_section_element;
        }
    }

    return nullptr;
}

DOM::ExceptionOr<void> HTMLTableElement::set_t_head(HTMLTableSectionElement* thead)
{
    VERIFY(thead);

    if (thead->local_name() != TagNames::thead)
        return DOM::HierarchyRequestError::create("Element is not thead");

    delete_t_head();

    DOM::Node* child_to_append_after = nullptr;
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableCaptionElement>(*child))
            continue;
        if (is<HTMLTableColElement>(*child)) {
            auto table_col_element = &verify_cast<HTMLTableColElement>(*child);
            if (table_col_element->local_name() == TagNames::colgroup)
                continue;
        }

        child_to_append_after = child;
        break;
    }

    pre_insert(*thead, child_to_append_after);

    return {};
}

NonnullRefPtr<HTMLTableSectionElement> HTMLTableElement::create_t_head()
{
    auto maybe_thead = t_head();
    if (maybe_thead)
        return *maybe_thead;

    auto thead = DOM::create_element(document(), TagNames::thead, Namespace::HTML);

    DOM::Node* child_to_append_after = nullptr;
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableCaptionElement>(*child))
            continue;
        if (is<HTMLTableColElement>(*child)) {
            auto table_col_element = &verify_cast<HTMLTableColElement>(*child);
            if (table_col_element->local_name() == TagNames::colgroup)
                continue;
        }

        child_to_append_after = child;
        break;
    }

    pre_insert(thead, child_to_append_after);

    return thead;
}

void HTMLTableElement::delete_t_head()
{
    auto maybe_thead = t_head();
    if (maybe_thead) {
        maybe_thead->remove(false);
    }
}

RefPtr<HTMLTableSectionElement> HTMLTableElement::t_foot()
{
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::tfoot)
                return table_section_element;
        }
    }

    return nullptr;
}

DOM::ExceptionOr<void> HTMLTableElement::set_t_foot(HTMLTableSectionElement* tfoot)
{
    VERIFY(tfoot);

    if (tfoot->local_name() != TagNames::tfoot)
        return DOM::HierarchyRequestError::create("Element is not tfoot");

    delete_t_foot();

    append_child(*tfoot);

    return {};
}

NonnullRefPtr<HTMLTableSectionElement> HTMLTableElement::create_t_foot()
{
    auto maybe_tfoot = t_foot();
    if (maybe_tfoot)
        return *maybe_tfoot;

    auto tfoot = DOM::create_element(document(), TagNames::tfoot, Namespace::HTML);
    append_child(tfoot);
    return tfoot;
}

void HTMLTableElement::delete_t_foot()
{
    auto maybe_tfoot = t_foot();
    if (maybe_tfoot) {
        maybe_tfoot->remove(false);
    }
}

NonnullRefPtr<DOM::HTMLCollection> HTMLTableElement::t_bodies()
{
    return DOM::HTMLCollection::create(*this, [](DOM::Element const& element) {
        return element.local_name() == TagNames::tbody;
    });
}

NonnullRefPtr<HTMLTableSectionElement> HTMLTableElement::create_t_body()
{
    auto tbody = DOM::create_element(document(), TagNames::tbody, Namespace::HTML);

    DOM::Node* child_to_append_after = nullptr;
    for (auto* child = last_child(); child; child = child->previous_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::tbody) {

                child_to_append_after = child->next_sibling();
                break;
            }
        }
    }

    pre_insert(tbody, child_to_append_after);

    return tbody;
}

NonnullRefPtr<DOM::HTMLCollection> HTMLTableElement::rows()
{
    HTMLTableElement* table_node = this;

    return DOM::HTMLCollection::create(*this, [table_node](DOM::Element const& element) {

        if (!is<HTMLTableRowElement>(element)) {
            return false;
        }
        if (element.parent_element() == table_node)
            return true;

        if (element.parent_element() && (element.parent_element()->local_name() == TagNames::thead || element.parent_element()->local_name() == TagNames::tbody || element.parent_element()->local_name() == TagNames::tfoot)
            && element.parent()->parent() == table_node) {
            return true;
        }

        return false;
    });
}

DOM::ExceptionOr<NonnullRefPtr<HTMLTableRowElement>> HTMLTableElement::insert_row(long index)
{
    auto rows = this->rows();
    auto rows_length = rows->length();

    if (index < -1 || index >= (long)rows_length) {
        return DOM::IndexSizeError::create("Index is negative or greater than the number of rows");
    }
    auto tr = static_cast<NonnullRefPtr<HTMLTableRowElement>>(DOM::create_element(document(), TagNames::tr, Namespace::HTML));
    if (rows_length == 0 && !has_child_of_type<HTMLTableRowElement>()) {
        auto tbody = DOM::create_element(document(), TagNames::tbody, Namespace::HTML);
        tbody->append_child(tr);
        append_child(tbody);
    } else if (rows_length == 0) {
        auto tbody = last_child_of_type<HTMLTableRowElement>();
        tbody->append_child(tr);
    } else if (index == -1 || index == (long)rows_length) {
        auto parent_of_last_tr = rows->item(rows_length - 1)->parent_element();
        parent_of_last_tr->append_child(tr);
    } else {
        rows->item(index)->parent_element()->insert_before(tr, rows->item(index));
    }
    return tr;
}

DOM::ExceptionOr<void> HTMLTableElement::delete_row(long index)
{
    auto rows = this->rows();
    auto rows_length = rows->length();

    if (index < -1 || index >= (long)rows_length) {
        return DOM::IndexSizeError::create("Index is negative or greater than the number of rows");
    }
    if (index == -1 && rows_length > 0) {
        auto row_to_remove = rows->item(rows_length - 1);
        row_to_remove->remove(false);
    } else {
        auto row_to_remove = rows->item(index);
        row_to_remove->remove(false);
    }

    return {};
}

}
