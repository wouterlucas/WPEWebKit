#include "config.h"
#include "PlatformTVTuner.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

RefPtr<PlatformTVTuner> PlatformTVTuner::create(String Id, PlatformTVControlBackend* tvBackend)
{
    return adoptRef(new PlatformTVTuner(Id, tvBackend));
}

PlatformTVTuner::PlatformTVTuner(String Id, PlatformTVControlBackend* tvBackend)
    : m_tunerId(Id)
    , m_platformTVTunerClient(nullptr)
    , m_tvBackend(tvBackend)
    , m_sourceTypeListIsInitialized(false)
    , m_sourceListIsInitialized(false)
{
}

PlatformTVTuner::~PlatformTVTuner()
{
}

void PlatformTVTuner::setTunerClient(PlatformTVTunerClient* client)
{
    m_platformTVTunerClient = client;
}

const Vector<PlatformTVSource::Type>& PlatformTVTuner::getSupportedSourceTypes()
{
    if (!m_sourceTypeListIsInitialized) {
        ASSERT(m_sourceListType.isEmpty());
        struct wpe_tvcontrol_string_vector sourceTypeList;
        wpe_tvcontrol_backend_get_supported_source_types_list(m_tvBackend->m_backend, m_tunerId.utf8().data(), &sourceTypeList);
        if (sourceTypeList.length) {
            for(uint64_t i = 0; i < sourceTypeList.length; i++) {
                String tmpType(sourceTypeList.strings[i].data, sourceTypeList.strings[i].length);
                //m_sourceTypeList.append(tmpType);
            }
            m_sourceTypeListIsInitialized = true;
        }
    }
    return m_sourceTypeList;
}

const Vector<RefPtr<PlatformTVSource>>& PlatformTVTuner::getSources()
{
    if (!m_sourceListIsInitialized) {
        ASSERT(m_sourceList.isEmpty());
        //Do steps to identify tuners;
        m_sourceListIsInitialized = true;
    }
    return m_sourceList;
}

void  PlatformTVTuner::setCurrentSource (PlatformTVSource::Type sourceType) {
    m_currentSourceType = sourceType;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
