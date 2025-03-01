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
#include "WebEventFactory.h"

#include <wpe/input.h>
#include <WebCore/Scrollbar.h>
#include <cstdlib>
#include <wtf/glib/GUniquePtr.h>

namespace WebKit {

static WebEvent::Modifiers modifiersForEvent(struct wpe_input_keyboard_event* event)
{
    unsigned modifiers = 0;
    unsigned eventModifiers = event->modifiers;

    if (eventModifiers & wpe_input_keyboard_modifier_control)
        modifiers |= WebEvent::ControlKey;
    if (eventModifiers & wpe_input_keyboard_modifier_shift)
        modifiers |= WebEvent::ShiftKey;
    if (eventModifiers & wpe_input_keyboard_modifier_alt)
        modifiers |= WebEvent::AltKey;
    if (eventModifiers & wpe_input_keyboard_modifier_meta)
        modifiers |= WebEvent::MetaKey;

    return static_cast<WebEvent::Modifiers>(modifiers);
}

static String singleCharacterStringForKeyEvent(struct wpe_input_keyboard_event* event)
{
    const char* singleCharacter = wpe_input_single_character_for_key_event(wpe_input_key_mapper_get_singleton(), event);
    if (singleCharacter)
        return String(singleCharacter);

    glong length;
    GUniquePtr<gunichar2> uchar16(g_ucs4_to_utf16(&event->unicode, 1, 0, &length, nullptr));
    if (uchar16)
        return String(uchar16.get());
    return String();
}

static String identifierStringForKeyEvent(struct wpe_input_keyboard_event* event)
{
    const char* identifier = wpe_input_identifier_for_key_event(wpe_input_key_mapper_get_singleton(), event);
    if (identifier)
        return String(identifier);

    return String::format("U+%04X", event->unicode);
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(struct wpe_input_keyboard_event* event)
{
    String singleCharacterString = singleCharacterStringForKeyEvent(event);
    String identifierString = identifierStringForKeyEvent(event);

    return WebKeyboardEvent(event->pressed ? WebEvent::KeyDown : WebEvent::KeyUp,
        singleCharacterString, singleCharacterString, identifierString,
        wpe_input_windows_key_code_for_key_event(wpe_input_key_mapper_get_singleton(), event),
        event->keyCode, 0, false, false, false,
        modifiersForEvent(event), event->time);
}

WebMouseEvent WebEventFactory::createWebMouseEvent(struct wpe_input_pointer_event* event)
{
    WebEvent::Type type = WebEvent::NoType;
    switch (event->type) {
    case wpe_input_pointer_event_type_motion:
        type = WebEvent::MouseMove;
        break;
    case wpe_input_pointer_event_type_button:
        type = event->state ? WebEvent::MouseDown : WebEvent::MouseUp;
        break;
    case wpe_input_pointer_event_type_null:
        ASSERT_NOT_REACHED();
    }

    WebMouseEvent::Button button = WebMouseEvent::NoButton;
    switch (event->type) {
    case wpe_input_pointer_event_type_motion:
    case wpe_input_pointer_event_type_button:
        if (event->button == 1)
            button = WebMouseEvent::LeftButton;
        else if (event->button == 2)
            button = WebMouseEvent::RightButton;
        else if (event->button == 3)
            button = WebMouseEvent::MiddleButton;
        break;
    case wpe_input_pointer_event_type_null:
        ASSERT_NOT_REACHED();
    }

    unsigned clickCount = (type == WebEvent::MouseDown) ? 1 : 0;

    // FIXME: Proper button support. Modifiers. deltaX/Y/Z. Click count.
    WebCore::IntPoint position(event->x, event->y);
    return WebMouseEvent(type, button, position, position,
        0, 0, 0, clickCount, static_cast<WebEvent::Modifiers>(0), event->time);
}

WebWheelEvent WebEventFactory::createWebWheelEvent(struct wpe_input_axis_event* event)
{
    // FIXME: We shouldn't hard-code this.
    enum Axis {
        Vertical,
        Horizontal
    };

    WebCore::FloatSize wheelTicks;
    switch (event->axis) {
    case Vertical:
        wheelTicks = WebCore::FloatSize(0, 1);
        break;
    case Horizontal:
        wheelTicks = WebCore::FloatSize(1, 0);
        break;
    default:
        ASSERT_NOT_REACHED();
    };

    wheelTicks.scale(event->value / std::abs(event->value));

    WebCore::FloatSize delta = wheelTicks;
    delta.scale(WebCore::Scrollbar::pixelsPerLineStep());

    WebCore::IntPoint position(event->x, event->y);
    return WebWheelEvent(WebEvent::Wheel, position, position,
        delta, wheelTicks, WebWheelEvent::ScrollByPixelWheelEvent, static_cast<WebEvent::Modifiers>(0), event->time);
}

#if ENABLE(TOUCH_EVENTS)
static WebKit::WebPlatformTouchPoint::TouchPointState stateForTouchPoint(int mainEventId, const struct wpe_input_touch_event_raw* point)
{
    if (point->id != mainEventId)
        return WebKit::WebPlatformTouchPoint::TouchStationary;

    switch (point->type) {
    case wpe_input_touch_event_type_down:
        return WebKit::WebPlatformTouchPoint::TouchPressed;
    case wpe_input_touch_event_type_motion:
        return WebKit::WebPlatformTouchPoint::TouchMoved;
    case wpe_input_touch_event_type_up:
        return WebKit::WebPlatformTouchPoint::TouchReleased;
    case wpe_input_touch_event_type_null:
        ASSERT_NOT_REACHED();
        break;
    };

    return WebKit::WebPlatformTouchPoint::TouchStationary;
}

WebTouchEvent WebEventFactory::createWebTouchEvent(struct wpe_input_touch_event* event)
{
    WebEvent::Type type = WebEvent::NoType;
    switch (event->type) {
    case wpe_input_touch_event_type_down:
        type = WebEvent::TouchStart;
        break;
    case wpe_input_touch_event_type_motion:
        type = WebEvent::TouchMove;
        break;
    case wpe_input_touch_event_type_up:
        type = WebEvent::TouchEnd;
        break;
    case wpe_input_touch_event_type_null:
        ASSERT_NOT_REACHED();
    }

    Vector<WebKit::WebPlatformTouchPoint> touchPoints;
    touchPoints.reserveCapacity(event->touchpoints_length);

    for (unsigned i = 0; i < event->touchpoints_length; ++i) {
        auto& point = event->touchpoints[i];
        if (point.type == wpe_input_touch_event_type_null)
            continue;

        touchPoints.uncheckedAppend(WebKit::WebPlatformTouchPoint(point.id, stateForTouchPoint(event->id, &point),
            WebCore::IntPoint(point.x, point.y), WebCore::IntPoint(point.x, point.y)));
    }

    return WebTouchEvent(type, WTFMove(touchPoints), WebEvent::Modifiers(0), event->time);
}
#endif // ENABLE(TOUCH_EVENTS)

} // namespace WebKit
