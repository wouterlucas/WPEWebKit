#ifndef PlatformTVTuner_h
#define PlatformTVTuner_h

#if ENABLE(TV_CONTROL)

#include "PlatformTVSource.h"

namespace WebCore {

class PlatformTVControlBackend;

class PlatformTVTunerClient {
public:
protected:
    virtual ~PlatformTVTunerClient() { }
};

class PlatformTVTuner : public RefCounted<PlatformTVTuner> {
public:
    static RefPtr<PlatformTVTuner> create(String, PlatformTVControlBackend*);

    virtual ~PlatformTVTuner();

    const Vector<PlatformTVSource::Type>&    getSupportedSourceTypes();
    const Vector<RefPtr<PlatformTVSource>>&  getSources ();
    RefPtr<PlatformTVSource>                 setCurrentSource (PlatformTVSource::Type sourceType);
    void                                     setTunerClient (PlatformTVTunerClient* client);

    const String&             id () const { return m_tunerId; }
    RefPtr<PlatformTVSource>  currentSource() const { return m_currentSource; } //TODO check again
    double                    signalStrength();
private:
    PlatformTVTuner(String, PlatformTVControlBackend*);
    String                     m_tunerId;
    RefPtr<PlatformTVSource>   m_currentSource;
    double                     m_signalStrength;
    PlatformTVSource::Type     m_currentSourceType;
    PlatformTVTunerClient*     m_platformTVTunerClient;
    PlatformTVControlBackend*  m_tvBackend;

    Vector<PlatformTVSource::Type> m_sourceTypeList;
    bool m_sourceTypeListIsInitialized;
    Vector<RefPtr<PlatformTVSource>> m_sourceList;
    bool m_sourceListIsInitialized;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)

#endif // PlatformTVTuner_h
