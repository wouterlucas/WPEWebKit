/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "WebPlaybackSessionInterfaceMac.h"

#if PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)

#import "AVKitSPI.h"
#import "IntRect.h"
#import "MediaTimeAVFoundation.h"
#import "TimeRanges.h"
#import "WebPlaybackControlsManager.h"
#import "WebPlaybackSessionModel.h"
#import <AVFoundation/AVFoundation.h>

#import "CoreMediaSoftLink.h"

SOFT_LINK_FRAMEWORK_OPTIONAL(AVKit)
SOFT_LINK_CLASS_OPTIONAL(AVKit, AVValueTiming)

using namespace WebCore;

namespace WebCore {

Ref<WebPlaybackSessionInterfaceMac> WebPlaybackSessionInterfaceMac::create(WebPlaybackSessionModel& model)
{
    auto interface = adoptRef(*new WebPlaybackSessionInterfaceMac(model));
    model.addClient(interface);
    return interface;
}

WebPlaybackSessionInterfaceMac::WebPlaybackSessionInterfaceMac(WebPlaybackSessionModel& model)
    : m_playbackSessionModel(&model)
{
}

WebPlaybackSessionInterfaceMac::~WebPlaybackSessionInterfaceMac()
{
    invalidate();
}

void WebPlaybackSessionInterfaceMac::durationChanged(double duration)
{
    WebPlaybackControlsManager* controlsManager = playBackControlsManager();

    controlsManager.contentDuration = duration;

    // FIXME: We take this as an indication that playback is ready, but that is not necessarily true.
    controlsManager.hasEnabledAudio = YES;
    controlsManager.hasEnabledVideo = YES;
}

void WebPlaybackSessionInterfaceMac::currentTimeChanged(double currentTime, double anchorTime)
{
    WebPlaybackControlsManager* controlsManager = playBackControlsManager();
    updatePlaybackControlsManagerTiming(currentTime, anchorTime, controlsManager.rate, controlsManager.playing);
}

void WebPlaybackSessionInterfaceMac::rateChanged(bool isPlaying, float playbackRate)
{
    WebPlaybackControlsManager* controlsManager = playBackControlsManager();
    [controlsManager setRate:isPlaying ? playbackRate : 0.];
    [controlsManager setPlaying:isPlaying];
    updatePlaybackControlsManagerTiming(m_playbackSessionModel ? m_playbackSessionModel->currentTime() : 0, [[NSProcessInfo processInfo] systemUptime], playbackRate, isPlaying);
}

void WebPlaybackSessionInterfaceMac::beginScrubbing()
{
    updatePlaybackControlsManagerTiming(m_playbackSessionModel ? m_playbackSessionModel->currentTime() : 0, [[NSProcessInfo processInfo] systemUptime], 0, false);
    webPlaybackSessionModel()->beginScrubbing();
}

void WebPlaybackSessionInterfaceMac::endScrubbing()
{
    webPlaybackSessionModel()->endScrubbing();
}

static RetainPtr<NSMutableArray> timeRangesToArray(const TimeRanges& timeRanges)
{
    RetainPtr<NSMutableArray> rangeArray = adoptNS([[NSMutableArray alloc] init]);

    for (unsigned i = 0; i < timeRanges.length(); i++) {
        const PlatformTimeRanges& ranges = timeRanges.ranges();
        CMTimeRange range = CMTimeRangeMake(toCMTime(ranges.start(i)), toCMTime(ranges.end(i)));
        [rangeArray addObject:[NSValue valueWithCMTimeRange:range]];
    }

    return rangeArray;
}

void WebPlaybackSessionInterfaceMac::seekableRangesChanged(const TimeRanges& timeRanges)
{
    [playBackControlsManager() setSeekableTimeRanges:timeRangesToArray(timeRanges).get()];
}

void WebPlaybackSessionInterfaceMac::audioMediaSelectionOptionsChanged(const Vector<WTF::String>& options, uint64_t selectedIndex)
{
    [playBackControlsManager() setAudioMediaSelectionOptions:options withSelectedIndex:static_cast<NSUInteger>(selectedIndex)];
}

void WebPlaybackSessionInterfaceMac::legibleMediaSelectionOptionsChanged(const Vector<WTF::String>& options, uint64_t selectedIndex)
{
    [playBackControlsManager() setLegibleMediaSelectionOptions:options withSelectedIndex:static_cast<NSUInteger>(selectedIndex)];
}

void WebPlaybackSessionInterfaceMac::invalidate()
{
    if (!m_playbackSessionModel)
        return;

    m_playbackSessionModel->removeClient(*this);
    m_playbackSessionModel = nullptr;
}

void WebPlaybackSessionInterfaceMac::ensureControlsManager()
{
    playBackControlsManager();
}

WebPlaybackControlsManager *WebPlaybackSessionInterfaceMac::playBackControlsManager()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
    return m_playbackControlsManager;
#else
    return nil;
#endif
}

void WebPlaybackSessionInterfaceMac::setPlayBackControlsManager(WebPlaybackControlsManager *manager)
{
    m_playbackControlsManager = manager;

    if (!manager || !m_playbackSessionModel)
        return;

    NSTimeInterval anchorTimeStamp = ![manager rate] ? NAN : [[NSProcessInfo processInfo] systemUptime];
    manager.timing = [getAVValueTimingClass() valueTimingWithAnchorValue:m_playbackSessionModel->currentTime() anchorTimeStamp:anchorTimeStamp rate:0];
    double duration = m_playbackSessionModel->duration();
    manager.contentDuration = duration;
    manager.hasEnabledAudio = duration > 0;
    manager.hasEnabledVideo = duration > 0;
    manager.rate = m_playbackSessionModel->isPlaying() ? m_playbackSessionModel->playbackRate() : 0.;
    manager.seekableTimeRanges = timeRangesToArray(m_playbackSessionModel->seekableRanges()).get();
    manager.canTogglePlayback = YES;
    manager.playing = m_playbackSessionModel->isPlaying();
    [manager setAudioMediaSelectionOptions:m_playbackSessionModel->audioMediaSelectionOptions() withSelectedIndex:static_cast<NSUInteger>(m_playbackSessionModel->audioMediaSelectedIndex())];
    [manager setLegibleMediaSelectionOptions:m_playbackSessionModel->legibleMediaSelectionOptions() withSelectedIndex:static_cast<NSUInteger>(m_playbackSessionModel->legibleMediaSelectedIndex())];
}

void WebPlaybackSessionInterfaceMac::updatePlaybackControlsManagerTiming(double currentTime, double anchorTime, double playbackRate, bool isPlaying)
{
    WebPlaybackControlsManager *manager = playBackControlsManager();
    if (!manager)
        return;

    WebPlaybackSessionModel *model = webPlaybackSessionModel();
    if (!model)
        return;

    double effectiveAnchorTime = playbackRate ? anchorTime : NAN;
    double effectivePlaybackRate = playbackRate;
    if (!isPlaying
        || model->isScrubbing()
        || (manager.rate > 0 && model->playbackStartedTime() >= currentTime)
        || (manager.rate < 0 && model->playbackStartedTime() <= currentTime))
        effectivePlaybackRate = 0;

    manager.timing = [getAVValueTimingClass() valueTimingWithAnchorValue:currentTime anchorTimeStamp:effectiveAnchorTime rate:effectivePlaybackRate];
}

}

#endif // PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)
