/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Company 100, Inc.
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
#include "ThreadedCoordinatedLayerTreeHost.h"

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "AcceleratedSurface.h"
#include "WebPage.h"
#include <WebCore/FrameView.h>
#include <WebCore/MainFrame.h>

using namespace WebCore;

namespace WebKit {

Ref<ThreadedCoordinatedLayerTreeHost> ThreadedCoordinatedLayerTreeHost::create(WebPage& webPage)
{
    return adoptRef(*new ThreadedCoordinatedLayerTreeHost(webPage));
}

ThreadedCoordinatedLayerTreeHost::~ThreadedCoordinatedLayerTreeHost()
{
}

ThreadedCoordinatedLayerTreeHost::ThreadedCoordinatedLayerTreeHost(WebPage& webPage)
    : CoordinatedLayerTreeHost(webPage)
    , m_compositorClient(*this)
    , m_surface(AcceleratedSurface::create(webPage))
{
    if (m_surface) {
        TextureMapper::PaintFlags paintFlags = 0;

        if (m_surface->shouldPaintMirrored())
            paintFlags |= TextureMapper::PaintingMirrored;

        // Do not do frame sync when rendering offscreen in the web process to ensure that SwapBuffers never blocks.
        // Rendering to the actual screen will happen later anyway since the UI process schedules a redraw for every update,
        // the compositor will take care of syncing to vblank.
        m_compositor = ThreadedCompositor::create(m_compositorClient, webPage, m_surface->window(), ThreadedCompositor::ShouldDoFrameSync::No, paintFlags);
        m_layerTreeContext.contextID = m_surface->surfaceID();
    } else
        m_compositor = ThreadedCompositor::create(m_compositorClient, webPage);
}

void ThreadedCoordinatedLayerTreeHost::invalidate()
{
    m_compositor->invalidate();
    CoordinatedLayerTreeHost::invalidate();
    m_surface = nullptr;
}

void ThreadedCoordinatedLayerTreeHost::forceRepaint()
{
    CoordinatedLayerTreeHost::forceRepaint();
    m_compositor->forceRepaint();
}

void ThreadedCoordinatedLayerTreeHost::scrollNonCompositedContents(const IntRect& rect)
{
    FrameView* frameView = m_webPage.mainFrameView();
    if (!frameView || !frameView->delegatesScrolling())
        return;

    m_viewportController.didScroll(rect.location());
    didChangeViewport();
}

void ThreadedCoordinatedLayerTreeHost::contentsSizeChanged(const IntSize& size)
{
    m_viewportController.didChangeViewportSize(size);
    IntSize scaledSize(size);
    scaledSize.scale(m_webPage.deviceScaleFactor());
    m_compositor->setViewportSize(scaledSize, m_webPage.deviceScaleFactor() * m_viewportController.pageScaleFactor());
    didChangeViewport();
}

void ThreadedCoordinatedLayerTreeHost::deviceOrPageScaleFactorChanged()
{
    if (m_surface && m_surface->resize(m_webPage.size()))
        m_layerTreeContext.contextID = m_surface->surfaceID();

    CoordinatedLayerTreeHost::deviceOrPageScaleFactorChanged();
    m_compositor->setScaleFactor(m_webPage.deviceScaleFactor() * m_viewportController.pageScaleFactor());
}

void ThreadedCoordinatedLayerTreeHost::pageBackgroundTransparencyChanged()
{
    CoordinatedLayerTreeHost::pageBackgroundTransparencyChanged();
    m_compositor->setDrawsBackground(m_webPage.drawsBackground());
}

void ThreadedCoordinatedLayerTreeHost::sizeDidChange(const IntSize& size)
{
    if (m_surface && m_surface->resize(size))
        m_layerTreeContext.contextID = m_surface->surfaceID();

    CoordinatedLayerTreeHost::sizeDidChange(size);
    m_viewportController.didChangeContentsSize(size);
}

void ThreadedCoordinatedLayerTreeHost::didChangeViewportProperties(const ViewportAttributes& attr)
{
    m_viewportController.didChangeViewportAttributes(attr);
    didChangeViewport();
}

#if PLATFORM(GTK) && PLATFORM(X11) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
void ThreadedCoordinatedLayerTreeHost::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    m_layerTreeContext.contextID = handle;
    m_compositor->setNativeSurfaceHandleForCompositing(handle);
    scheduleLayerFlush();
}
#endif

void ThreadedCoordinatedLayerTreeHost::didChangeViewport()
{
    FloatRect visibleRect(m_viewportController.visibleContentsRect());
    if (visibleRect.isEmpty())
        return;

    // When using non overlay scrollbars, the contents size doesn't include the scrollbars, but we need to include them
    // in the visible area used by the compositor to ensure that the scrollbar layers are also updated.
    // See https://bugs.webkit.org/show_bug.cgi?id=160450.
    FrameView* view = m_webPage.corePage()->mainFrame().view();
    Scrollbar* scrollbar = view->verticalScrollbar();
    if (scrollbar && !scrollbar->isOverlayScrollbar())
        visibleRect.expand(scrollbar->width(), 0);
    scrollbar = view->horizontalScrollbar();
    if (scrollbar && !scrollbar->isOverlayScrollbar())
        visibleRect.expand(0, scrollbar->height());

    CoordinatedLayerTreeHost::setVisibleContentsRect(visibleRect, FloatPoint::zero());

    float pageScale = m_viewportController.pageScaleFactor();
    IntPoint scrollPosition = roundedIntPoint(visibleRect.location());
    if (m_lastScrollPosition != scrollPosition) {
        m_lastScrollPosition = scrollPosition;
        m_compositor->setScrollPosition(m_lastScrollPosition, m_webPage.deviceScaleFactor() * pageScale);

        if (!view->useFixedLayout())
            view->notifyScrollPositionChanged(m_lastScrollPosition);
    }

    if (m_lastPageScaleFactor != pageScale) {
        m_lastPageScaleFactor = pageScale;
        m_webPage.scalePage(pageScale, m_lastScrollPosition);
    }
}

void ThreadedCoordinatedLayerTreeHost::commitSceneState(const CoordinatedGraphicsState& state)
{
    CoordinatedLayerTreeHost::commitSceneState(state);
    m_compositor->updateSceneState(state);
}

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
RefPtr<WebCore::DisplayRefreshMonitor> ThreadedCoordinatedLayerTreeHost::createDisplayRefreshMonitor(PlatformDisplayID displayID)
{
    return m_compositor->createDisplayRefreshMonitor(displayID);
}
#endif

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
