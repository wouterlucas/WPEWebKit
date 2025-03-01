/*
 * Copyright (C) 2011-2013 University of Washington. All rights reserved.
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorReplayAgent.h"

#if ENABLE(WEB_REPLAY)

#include "DocumentLoader.h"
#include "Event.h"
#include "EventLoopInput.h"
#include "Frame.h"
#include "FunctorInputCursor.h"
#include "InspectorController.h"
#include "InspectorPageAgent.h"
#include <inspector/InspectorProtocolObjects.h>
#include "InstrumentingAgents.h"
#include "Logging.h"
#include "Page.h"
#include "ReplayController.h"
#include "ReplaySession.h"
#include "ReplaySessionSegment.h"
#include "SerializationMethods.h"
#include "WebReplayInputs.h" // For EncodingTraits<InputQueue>.
#include <inspector/InspectorValues.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace Inspector;

namespace WebCore {

static Ref<Inspector::Protocol::Replay::ReplayPosition> buildInspectorObjectForPosition(const ReplayPosition& position)
{
    return Inspector::Protocol::Replay::ReplayPosition::create()
        .setSegmentOffset(position.segmentOffset)
        .setInputOffset(position.inputOffset)
        .release();
}

static Ref<Inspector::Protocol::Replay::ReplayInput> buildInspectorObjectForInput(const NondeterministicInputBase& input, size_t offset)
{
    EncodedValue encodedInput = EncodingTraits<NondeterministicInputBase>::encodeValue(input);
    return Inspector::Protocol::Replay::ReplayInput::create()
        .setType(input.type())
        .setOffset(offset)
        .setData(encodedInput.asObject())
        .release();
}

static Ref<Inspector::Protocol::Replay::ReplaySession> buildInspectorObjectForSession(RefPtr<ReplaySession>&& session)
{
    auto segments = Inspector::Protocol::Array<SegmentIdentifier>::create();

    for (auto& segment : *session)
        segments->addItem(static_cast<int>(segment->identifier()));

    return Inspector::Protocol::Replay::ReplaySession::create()
        .setId(session->identifier())
        .setTimestamp(session->timestamp())
        .setSegments(WTFMove(segments))
        .release();
}

static Inspector::Protocol::Replay::SessionState buildInspectorObjectForSessionState(WebCore::SessionState sessionState)
{
    switch (sessionState) {
    case WebCore::SessionState::Capturing: return Inspector::Protocol::Replay::SessionState::Capturing;
    case WebCore::SessionState::Inactive: return Inspector::Protocol::Replay::SessionState::Inactive;
    case WebCore::SessionState::Replaying: return Inspector::Protocol::Replay::SessionState::Replaying;
    }

    RELEASE_ASSERT_NOT_REACHED();
    return Inspector::Protocol::Replay::SessionState::Inactive;
}

static Inspector::Protocol::Replay::SegmentState buildInspectorObjectForSegmentState(WebCore::SegmentState segmentState)
{
    switch (segmentState) {
    case WebCore::SegmentState::Appending: return Inspector::Protocol::Replay::SegmentState::Appending;
    case WebCore::SegmentState::Unloaded: return Inspector::Protocol::Replay::SegmentState::Unloaded;
    case WebCore::SegmentState::Loaded: return Inspector::Protocol::Replay::SegmentState::Loaded;
    case WebCore::SegmentState::Dispatching: return Inspector::Protocol::Replay::SegmentState::Dispatching;
    }

    RELEASE_ASSERT_NOT_REACHED();
    return Inspector::Protocol::Replay::SegmentState::Unloaded;
}

class SerializeInputToJSONFunctor {
public:
    typedef RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Replay::ReplayInput>> ReturnType;

    SerializeInputToJSONFunctor()
        : m_inputs(Inspector::Protocol::Array<Inspector::Protocol::Replay::ReplayInput>::create()) { }
    ~SerializeInputToJSONFunctor() { }

    void operator()(size_t index, const NondeterministicInputBase* input)
    {
        LOG(WebReplay, "%-25s Writing %5zu: %s\n", "[SerializeInput]", index, input->type().ascii().data());

        if (RefPtr<Inspector::Protocol::Replay::ReplayInput> serializedInput = buildInspectorObjectForInput(*input, index))
            m_inputs->addItem(WTFMove(serializedInput));
    }

    ReturnType returnValue() { return WTFMove(m_inputs); }
private:
    RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Replay::ReplayInput>> m_inputs;
};

static Ref<Inspector::Protocol::Replay::SessionSegment> buildInspectorObjectForSegment(RefPtr<ReplaySessionSegment>&& segment)
{
    auto queuesObject = Inspector::Protocol::Array<Inspector::Protocol::Replay::ReplayInputQueue>::create();

    for (size_t i = 0; i < static_cast<size_t>(InputQueue::Count); i++) {
        SerializeInputToJSONFunctor collector;
        InputQueue queue = static_cast<InputQueue>(i);
        RefPtr<FunctorInputCursor> functorCursor = FunctorInputCursor::create(segment.copyRef());
        RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Replay::ReplayInput>> queueInputs = functorCursor->forEachInputInQueue(queue, collector);

        auto queueObject = Inspector::Protocol::Replay::ReplayInputQueue::create()
            .setType(EncodingTraits<InputQueue>::encodeValue(queue).convertTo<String>())
            .setInputs(queueInputs)
            .release();
        queuesObject->addItem(WTFMove(queueObject));
    }

    return Inspector::Protocol::Replay::SessionSegment::create()
        .setId(segment->identifier())
        .setTimestamp(segment->timestamp())
        .setQueues(WTFMove(queuesObject))
        .release();
}

InspectorReplayAgent::InspectorReplayAgent(PageAgentContext& context)
    : InspectorAgentBase(ASCIILiteral("Replay"), context)
    , m_frontendDispatcher(std::make_unique<Inspector::ReplayFrontendDispatcher>(context.frontendRouter))
    , m_backendDispatcher(Inspector::ReplayBackendDispatcher::create(context.backendDispatcher, this))
    , m_page(context.inspectedPage)
{
}

InspectorReplayAgent::~InspectorReplayAgent()
{
    ASSERT(!m_sessionsMap.size());
    ASSERT(!m_segmentsMap.size());
}

WebCore::SessionState InspectorReplayAgent::sessionState() const
{
    return m_page.replayController().sessionState();
}

void InspectorReplayAgent::didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*)
{
    m_instrumentingAgents.setInspectorReplayAgent(this);
    ASSERT(sessionState() == WebCore::SessionState::Inactive);

    // Keep track of the (default) session currently loaded by ReplayController,
    // and any segments within the session.
    RefPtr<ReplaySession> session = m_page.replayController().loadedSession();
    m_sessionsMap.add(session->identifier(), session);

    for (auto& segment : *session)
        m_segmentsMap.add(segment->identifier(), segment);
}

void InspectorReplayAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    m_instrumentingAgents.setInspectorReplayAgent(nullptr);

    // Drop references to all sessions and segments.
    m_sessionsMap.clear();
    m_segmentsMap.clear();
}

void InspectorReplayAgent::frameNavigated(Frame& frame)
{
    if (sessionState() != WebCore::SessionState::Inactive)
        m_page.replayController().frameNavigated(frame);
}

void InspectorReplayAgent::frameDetached(Frame& frame)
{
    if (sessionState() != WebCore::SessionState::Inactive)
        m_page.replayController().frameDetached(frame);
}

void InspectorReplayAgent::willDispatchEvent(const Event& event, Frame* frame)
{
    if (sessionState() != WebCore::SessionState::Inactive)
        m_page.replayController().willDispatchEvent(event, frame);
}

void InspectorReplayAgent::sessionCreated(RefPtr<ReplaySession>&& session)
{
    auto result = m_sessionsMap.add(session->identifier(), session);
    // Can't have two sessions with same identifier.
    ASSERT_UNUSED(result, result.isNewEntry);

    m_frontendDispatcher->sessionCreated(session->identifier());
}

void InspectorReplayAgent::sessionModified(RefPtr<ReplaySession>&& session)
{
    m_frontendDispatcher->sessionModified(session->identifier());
}

void InspectorReplayAgent::sessionLoaded(RefPtr<ReplaySession>&& session)
{
    // In case we didn't know about the loaded session, add here.
    m_sessionsMap.add(session->identifier(), session);

    m_frontendDispatcher->sessionLoaded(session->identifier());
}

void InspectorReplayAgent::segmentCreated(RefPtr<ReplaySessionSegment>&& segment)
{
    auto result = m_segmentsMap.add(segment->identifier(), segment);
    // Can't have two segments with the same identifier.
    ASSERT_UNUSED(result, result.isNewEntry);

    m_frontendDispatcher->segmentCreated(segment->identifier());
}

void InspectorReplayAgent::segmentCompleted(RefPtr<ReplaySessionSegment>&& segment)
{
    m_frontendDispatcher->segmentCompleted(segment->identifier());
}

void InspectorReplayAgent::segmentLoaded(RefPtr<ReplaySessionSegment>&& segment)
{
    // In case we didn't know about the loaded segment, add here.
    m_segmentsMap.add(segment->identifier(), segment.copyRef());

    m_frontendDispatcher->segmentLoaded(segment->identifier());
}

void InspectorReplayAgent::segmentUnloaded()
{
    m_frontendDispatcher->segmentUnloaded();
}

void InspectorReplayAgent::captureStarted()
{
    LOG(WebReplay, "-----CAPTURE START-----");

    m_frontendDispatcher->captureStarted();
}

void InspectorReplayAgent::captureStopped()
{
    LOG(WebReplay, "-----CAPTURE STOP-----");

    m_frontendDispatcher->captureStopped();
}

void InspectorReplayAgent::playbackStarted()
{
    LOG(WebReplay, "-----REPLAY START-----");

    m_frontendDispatcher->playbackStarted();
}

void InspectorReplayAgent::playbackPaused(const ReplayPosition& position)
{
    LOG(WebReplay, "-----REPLAY PAUSED-----");

    m_frontendDispatcher->playbackPaused(buildInspectorObjectForPosition(position));
}

void InspectorReplayAgent::playbackHitPosition(const ReplayPosition& position)
{
    m_frontendDispatcher->playbackHitPosition(buildInspectorObjectForPosition(position), monotonicallyIncreasingTime());
}

void InspectorReplayAgent::playbackFinished()
{
    LOG(WebReplay, "-----REPLAY FINISHED-----");

    m_frontendDispatcher->playbackFinished();
}

void InspectorReplayAgent::startCapturing(ErrorString& errorString)
{
    if (sessionState() != WebCore::SessionState::Inactive) {
        errorString = ASCIILiteral("Can't start capturing if the session is already capturing or replaying.");
        return;
    }

    m_page.replayController().startCapturing();
}

void InspectorReplayAgent::stopCapturing(ErrorString& errorString)
{
    if (sessionState() != WebCore::SessionState::Capturing) {
        errorString = ASCIILiteral("Can't stop capturing if capture is not in progress.");
        return;
    }

    m_page.replayController().stopCapturing();
}

void InspectorReplayAgent::replayToPosition(ErrorString& errorString, const InspectorObject& positionObject, bool fastReplay)
{
    ReplayPosition position;
    if (!positionObject.getInteger(ASCIILiteral("segmentOffset"), position.segmentOffset)) {
        errorString = ASCIILiteral("Couldn't decode ReplayPosition segment offset provided to ReplayAgent.replayToPosition.");
        return;
    }

    if (!positionObject.getInteger(ASCIILiteral("inputOffset"), position.inputOffset)) {
        errorString = ASCIILiteral("Couldn't decode ReplayPosition input offset provided to ReplayAgent.replayToPosition.");
        return;
    }

    if (sessionState() == WebCore::SessionState::Capturing) {
        errorString = ASCIILiteral("Can't start replay while capture is in progress.");
        return;
    }

    m_page.replayController().replayToPosition(position, (fastReplay) ? DispatchSpeed::FastForward : DispatchSpeed::RealTime);
}

void InspectorReplayAgent::replayToCompletion(ErrorString& errorString, bool fastReplay)
{
    if (sessionState() == WebCore::SessionState::Capturing) {
        errorString = ASCIILiteral("Can't start replay while capture is in progress.");
        return;
    }

    m_page.replayController().replayToCompletion((fastReplay) ? DispatchSpeed::FastForward : DispatchSpeed::RealTime);
}

void InspectorReplayAgent::pausePlayback(ErrorString& errorString)
{
    if (sessionState() != WebCore::SessionState::Replaying) {
        errorString = ASCIILiteral("Can't pause playback if playback is not in progress.");
        return;
    }

    m_page.replayController().pausePlayback();
}

void InspectorReplayAgent::cancelPlayback(ErrorString& errorString)
{
    if (sessionState() == WebCore::SessionState::Capturing) {
        errorString = ASCIILiteral("Can't cancel playback if capture is in progress.");
        return;
    }

    m_page.replayController().cancelPlayback();
}

void InspectorReplayAgent::switchSession(ErrorString& errorString, Inspector::Protocol::Replay::SessionIdentifier identifier)
{
    ASSERT_ARG(identifier, identifier > 0);

    if (sessionState() != WebCore::SessionState::Inactive) {
        errorString = ASCIILiteral("Can't switch sessions unless the session is neither capturing or replaying.");
        return;
    }

    RefPtr<ReplaySession> session = findSession(errorString, identifier);
    if (!session)
        return;

    m_page.replayController().switchSession(WTFMove(session));
}

void InspectorReplayAgent::insertSessionSegment(ErrorString& errorString, Inspector::Protocol::Replay::SessionIdentifier sessionIdentifier, SegmentIdentifier segmentIdentifier, int segmentIndex)
{
    ASSERT_ARG(sessionIdentifier, sessionIdentifier > 0);
    ASSERT_ARG(segmentIdentifier, segmentIdentifier > 0);
    ASSERT_ARG(segmentIndex, segmentIndex >= 0);

    RefPtr<ReplaySession> session = findSession(errorString, sessionIdentifier);
    RefPtr<ReplaySessionSegment> segment = findSegment(errorString, segmentIdentifier);

    if (!session || !segment)
        return;

    if (static_cast<size_t>(segmentIndex) > session->size()) {
        errorString = ASCIILiteral("Invalid segment index.");
        return;
    }

    if (session == m_page.replayController().loadedSession() && sessionState() != WebCore::SessionState::Inactive) {
        errorString = ASCIILiteral("Can't modify a loaded session unless the session is inactive.");
        return;
    }

    session->insertSegment(segmentIndex, WTFMove(segment));
    sessionModified(WTFMove(session));
}

void InspectorReplayAgent::removeSessionSegment(ErrorString& errorString, Inspector::Protocol::Replay::SessionIdentifier identifier, int segmentIndex)
{
    ASSERT_ARG(identifier, identifier > 0);
    ASSERT_ARG(segmentIndex, segmentIndex >= 0);

    RefPtr<ReplaySession> session = findSession(errorString, identifier);

    if (!session)
        return;

    if (static_cast<size_t>(segmentIndex) >= session->size()) {
        errorString = ASCIILiteral("Invalid segment index.");
        return;
    }

    if (session == m_page.replayController().loadedSession() && sessionState() != WebCore::SessionState::Inactive) {
        errorString = ASCIILiteral("Can't modify a loaded session unless the session is inactive.");
        return;
    }

    session->removeSegment(segmentIndex);
    sessionModified(WTFMove(session));
}

RefPtr<ReplaySession> InspectorReplayAgent::findSession(ErrorString& errorString, SessionIdentifier identifier)
{
    ASSERT_ARG(identifier, identifier > 0);

    auto it = m_sessionsMap.find(identifier);
    if (it == m_sessionsMap.end()) {
        errorString = ASCIILiteral("Couldn't find session with specified identifier");
        return nullptr;
    }

    return it->value;
}

RefPtr<ReplaySessionSegment> InspectorReplayAgent::findSegment(ErrorString& errorString, SegmentIdentifier identifier)
{
    ASSERT_ARG(identifier, identifier > 0);

    auto it = m_segmentsMap.find(identifier);
    if (it == m_segmentsMap.end()) {
        errorString = ASCIILiteral("Couldn't find segment with specified identifier");
        return nullptr;
    }

    return it->value;
}

void InspectorReplayAgent::currentReplayState(ErrorString&, Inspector::Protocol::Replay::SessionIdentifier* sessionIdentifier, Inspector::Protocol::OptOutput<Inspector::Protocol::Replay::SegmentIdentifier>* segmentIdentifier, Inspector::Protocol::Replay::SessionState* sessionState, Inspector::Protocol::Replay::SegmentState* segmentState, RefPtr<Inspector::Protocol::Replay::ReplayPosition>& replayPosition)
{
    *sessionState = buildInspectorObjectForSessionState(m_page.replayController().sessionState());
    *segmentState = buildInspectorObjectForSegmentState(m_page.replayController().segmentState());

    *sessionIdentifier = m_page.replayController().loadedSession()->identifier();
    if (m_page.replayController().loadedSegment())
        *segmentIdentifier = m_page.replayController().loadedSegment()->identifier();

    replayPosition = buildInspectorObjectForPosition(m_page.replayController().currentPosition());
}

void InspectorReplayAgent::getAvailableSessions(ErrorString&, RefPtr<Inspector::Protocol::Array<SessionIdentifier>>& sessionsList)
{
    sessionsList = Inspector::Protocol::Array<Inspector::Protocol::Replay::SessionIdentifier>::create();
    for (auto& pair : m_sessionsMap)
        sessionsList->addItem(pair.key);
}

void InspectorReplayAgent::getSessionData(ErrorString& errorString, Inspector::Protocol::Replay::SessionIdentifier identifier, RefPtr<Inspector::Protocol::Replay::ReplaySession>& serializedObject)
{
    RefPtr<ReplaySession> session = findSession(errorString, identifier);
    if (!session) {
        errorString = ASCIILiteral("Couldn't find the specified session.");
        return;
    }

    serializedObject = buildInspectorObjectForSession(WTFMove(session));
}

void InspectorReplayAgent::getSegmentData(ErrorString& errorString, Inspector::Protocol::Replay::SegmentIdentifier identifier, RefPtr<Inspector::Protocol::Replay::SessionSegment>& serializedObject)
{
    RefPtr<ReplaySessionSegment> segment = findSegment(errorString, identifier);
    if (!segment) {
        errorString = ASCIILiteral("Couldn't find the specified segment.");
        return;
    }

    serializedObject = buildInspectorObjectForSegment(WTFMove(segment));
}

} // namespace WebCore
#endif // ENABLE(WEB_REPLAY)
