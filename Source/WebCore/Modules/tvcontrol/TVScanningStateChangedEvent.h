#ifndef TVScanningStateChangedEvent_h
#define TVScanningStateChangedEvent_h

#if ENABLE(TV_CONTROL)

#include "Event.h"

namespace WebCore {

class TVChannel;

class TVScanningStateChangedEvent : public Event {
public:

    enum class State  {
        Cleared,
        Scanned,
        Completed,
        Stopped
    };

    static Ref<TVScanningStateChangedEvent> create (const AtomicString&, State, TVChannel*);
    ~TVScanningStateChangedEvent ();

    State         state () const { return m_state; }
    TVChannel*    channel () const { return m_channel; }

private:
    TVScanningStateChangedEvent (const AtomicString&, State, TVChannel*);

    State m_state;
    TVChannel* m_channel;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)

#endif // TVScanningStateChangedEvent_h
