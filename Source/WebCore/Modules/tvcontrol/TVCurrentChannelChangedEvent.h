#ifndef TVCurrentChannelChangedEvent_h
#define TVCurrentChannelChangedEvent_h

#if ENABLE(TV_CONTROL)

#include "Event.h"

namespace WebCore {

class TVChannel;

class TVCurrentChannelChangedEvent : public Event {
public:

    static Ref<TVCurrentChannelChangedEvent> create (const AtomicString&, TVChannel*);
    ~TVCurrentChannelChangedEvent ();

    TVChannel*                               channel () const { return m_channel; }

private:
    TVCurrentChannelChangedEvent (const AtomicString&, TVChannel*);

    TVChannel*   m_channel;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)

#endif // TVCurrentChannelChangedEvent_h
