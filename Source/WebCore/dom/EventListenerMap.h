/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include "RegisteredEventListener.h"
#include <atomic>
#include <memory>
#include <wtf/Forward.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class EventTarget;

using EventListenerVector = Vector<RefPtr<RegisteredEventListener>, 1>;

class EventListenerMap {
public:
    EventListenerMap();

    bool isEmpty() const { return m_entries.isEmpty(); }
    bool contains(const AtomicString& eventType) const { return find(eventType); }
    bool containsCapturing(const AtomicString& eventType) const;
    bool containsActive(const AtomicString& eventType) const;

    void clear();

    void replace(const AtomicString& eventType, EventListener& oldListener, Ref<EventListener>&& newListener, const RegisteredEventListener::Options&);
    bool add(const AtomicString& eventType, Ref<EventListener>&&, const RegisteredEventListener::Options&);
    bool remove(const AtomicString& eventType, EventListener&, bool useCapture);
    WEBCORE_EXPORT EventListenerVector* find(const AtomicString& eventType) const;
    Vector<AtomicString> eventTypes() const;

    void removeFirstEventListenerCreatedFromMarkup(const AtomicString& eventType);
    void copyEventListenersNotCreatedFromMarkupToTarget(EventTarget*);

private:
    friend class EventListenerIterator;

    void assertNoActiveIterators() const;

    Vector<std::pair<AtomicString, std::unique_ptr<EventListenerVector>>, 2> m_entries;

#ifndef NDEBUG
    std::atomic<int> m_activeIteratorCount { 0 };
#endif
};

class EventListenerIterator {
    WTF_MAKE_NONCOPYABLE(EventListenerIterator);
public:
    explicit EventListenerIterator(EventTarget*);
#ifndef NDEBUG
    ~EventListenerIterator();
#endif

    EventListener* nextListener();

private:
    EventListenerMap* m_map { nullptr };
    unsigned m_entryIndex { 0 };
    unsigned m_index { 0 };
};

#ifdef NDEBUG
inline void EventListenerMap::assertNoActiveIterators() const { }
#endif

} // namespace WebCore
