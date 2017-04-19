#include "config.h"
#include "PlatformTVManager.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

PlatformTVManager::PlatformTVManager(PlatformTVManagerClient* client)
    : m_tunerListIsInitialized(false)
    , m_platformTVManagerClient(client)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    m_tvBackend = new PlatformTVControlBackend();
    m_tvBackend->m_backend = wpe_tvcontrol_backend_create();

    static struct wpe_tvcontrol_backend_manager_event_client s_eventClient = {
        // handle_tuner_event
        [](void* data, wpe_tvcontrol_tuner_event event)
        {
            auto& tvManager = *reinterpret_cast<PlatformTVManager*>(data);
            tvManager.m_platformTVManagerClient->didTunerOperationChanged(String(event.tuner_id.data, event.tuner_id.length), (uint16_t)event.operation);
        },
    };
    wpe_tvcontrol_backend_set_manager_event_client(m_tvBackend->m_backend, &s_eventClient, this);
}

PlatformTVManager::~PlatformTVManager()
{
    wpe_tvcontrol_backend_destroy(m_tvBackend->m_backend);
    delete m_tvBackend;
}

const Vector<RefPtr<PlatformTVTuner>>& PlatformTVManager::getTuners()
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (!m_tunerListIsInitialized) {
        printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
        ASSERT(m_tunerList.isEmpty());

        struct wpe_tvcontrol_string_vector tunerList;
        tunerList.length = 0;
        wpe_tvcontrol_backend_get_tuner_list(m_tvBackend->m_backend, &tunerList);

        printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
        if (tunerList.length) {
            for(uint64_t i = 0; i < tunerList.length; i++) {
                String tmpId(tunerList.strings[i].data, tunerList.strings[i].length);
                m_tunerList.append(PlatformTVTuner::create(tmpId, m_tvBackend));
            }
            m_tunerListIsInitialized = true;
        }
    }
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return m_tunerList;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
