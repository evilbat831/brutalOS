/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// includes
#include "LoadRequest.h"
#include <libweb/cookie/Cookie.h>
#include <libweb/page/Page.h>

namespace Web {

LoadRequest LoadRequest::create_for_url_on_page(const URL& url, Page* page)
{
    LoadRequest request;
    request.set_url(url);

    if (page) {
        String cookie = page->client().page_did_request_cookie(url, Cookie::Source::Http);
        if (!cookie.is_empty())
            request.set_header("Cookie", cookie);
    }

    return request;
}

}