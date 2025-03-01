/*
 * Copyright (C) 2014 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WPEView.h"

#include "APIPageConfiguration.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif
#include "NativeWebWheelEvent.h"
#include "WebPageGroup.h"
#include "WebProcessPool.h"
#include <stdlib.h>
#include <wpe/view-backend.h>

using namespace WebKit;

namespace WKWPE {

View::View(struct wpe_view_backend* backend, const API::PageConfiguration& baseConfiguration)
    : m_pageClient(std::make_unique<PageClientImpl>(*this))
    , m_size{ 800, 600 }
    , m_viewStateFlags(WebCore::ActivityState::WindowIsActive | WebCore::ActivityState::IsFocused | WebCore::ActivityState::IsVisible | WebCore::ActivityState::IsInWindow)
    , m_compositingManagerProxy(*this)
{
    auto configuration = baseConfiguration.copy();
    auto* preferences = configuration->preferences();
    if (!preferences && configuration->pageGroup()) {
        preferences = &configuration->pageGroup()->preferences();
        configuration->setPreferences(preferences);
    }
    if (preferences) {
        preferences->setAcceleratedCompositingEnabled(true);
        preferences->setForceCompositingMode(true);
        preferences->setAccelerated2dCanvasEnabled(true);
        preferences->setWebGLEnabled(true);
        preferences->setWebSecurityEnabled(false);
        preferences->setDeveloperExtrasEnabled(true);
    }

    auto* pool = configuration->processPool();
    m_pageProxy = pool->createWebPage(*m_pageClient, WTFMove(configuration));

#if PLATFORM(INTEL_CE)
    m_pageProxy->setDrawsBackground(false);
#endif

    m_backend = backend;
    if (!m_backend)
        m_backend = wpe_view_backend_create();

    if (!m_backend) {
        fprintf(stderr, "WPEView: failed to create backend\n");
        abort();
    }

    m_compositingManagerProxy.initialize();

    static struct wpe_view_backend_client s_backendClient = {
        // set_size
        [](void* data, uint32_t width, uint32_t height)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.setSize(WebCore::IntSize(width, height));
        },
        // frame_displayed
        [](void* data)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.frameDisplayed();
        }
    };
    wpe_view_backend_set_backend_client(m_backend, &s_backendClient, this);

    static struct wpe_view_backend_input_client s_inputClient = {
        // handle_keyboard_event
        [](void* data, struct wpe_input_keyboard_event* event)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.page().handleKeyboardEvent(WebKit::NativeWebKeyboardEvent(event));
        },
        // handle_pointer_event
        [](void* data, struct wpe_input_pointer_event* event)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.page().handleMouseEvent(WebKit::NativeWebMouseEvent(event));
        },
        // handle_axis_event
        [](void* data, struct wpe_input_axis_event* event)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.page().handleWheelEvent(WebKit::NativeWebWheelEvent(event));
        },
#if ENABLE(TOUCH_EVENTS)
        // handle_touch_event
        [](void* data, struct wpe_input_touch_event* event)
        {
            auto& view = *reinterpret_cast<View*>(data);
            view.page().handleTouchEvent(WebKit::NativeWebTouchEvent(event));
        },
#endif
    };
    wpe_view_backend_set_input_client(m_backend, &s_inputClient, this);

    wpe_view_backend_initialize(m_backend);

    m_pageProxy->initializeWebPage();
}

View::~View()
{
    wpe_view_backend_destroy(m_backend);
}

void View::initializeClient(const WKViewClientBase* client)
{
    m_client.initialize(client);
}

void View::frameDisplayed()
{
    m_client.frameDisplayed(*this);
}

void View::setSize(const WebCore::IntSize& size)
{
    m_size = size;
    if (m_pageProxy->drawingArea())
        m_pageProxy->drawingArea()->setSize(size, WebCore::IntSize(), WebCore::IntSize());
}

void View::setViewState(WebCore::ActivityState::Flags flags)
{
    static const WebCore::ActivityState::Flags defaultFlags = WebCore::ActivityState::WindowIsActive | WebCore::ActivityState::IsFocused;

    WebCore::ActivityState::Flags changedFlags = m_viewStateFlags ^ (defaultFlags | flags);
    m_viewStateFlags = defaultFlags | flags;

    if (changedFlags)
        m_pageProxy->activityStateDidChange(changedFlags);
}

} // namespace WKWPE
