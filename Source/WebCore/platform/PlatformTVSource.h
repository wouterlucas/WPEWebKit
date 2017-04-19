#ifndef PlatformTVSource_h
#define PlatformTVSource_h

#if ENABLE(TV_CONTROL)

#include "PlatformTVChannel.h"

namespace WebCore {

class PlatformTVControlBackend;

class PlatformTVSourceClient {
public:
protected:
    virtual ~PlatformTVSourceClient() { }
};


class PlatformTVSource : public RefCounted<PlatformTVSource> {
public:
    enum class Type { DvbT, DvbT2, DvbC, DvbC2, DvbS, DvbS2, DvbH, DvbSh, Atsc, AtscMH, IsdbT, IsdbTb, IsdbS, IsdbC, _1seg, Dtmb, Cmmb, TDmb, SDmb, Undefined };

    static RefPtr<PlatformTVSource> create(PlatformTVControlBackend*, String, Type);
    virtual ~PlatformTVSource();

    const Vector<RefPtr<PlatformTVChannel>>&   getChannels ();
    RefPtr<PlatformTVChannel>                  setCurrentChannel (const String& channelNumber);
    void                                       startScanning (bool scanningOption);
    void                                       stopScanning ();
    void                                       setSourceClient (PlatformTVSourceClient* client);

    Type                                       type () const { return m_type; }

private:
    PlatformTVSource (PlatformTVControlBackend*, String, Type);
    String                      m_tunerId;
    Type                        m_type;
    RefPtr<PlatformTVChannel>   m_currentChannel;
    PlatformTVControlBackend*   m_tvBackend;
    PlatformTVSourceClient*     m_platformTVSourceClient;

    Vector<RefPtr<PlatformTVChannel>> m_channelList;
    bool m_channelListIsInitialized;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)

#endif // PlatformTVSource_h
