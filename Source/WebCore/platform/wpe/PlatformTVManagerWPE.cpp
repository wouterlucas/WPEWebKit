#include "config.h"
#include "PlatformTVManager.h"
#include <wtf/MainThread.h>

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
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tuner_id.data, event->tuner_id.length);
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            uint16_t operation = event->operation;
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, tunerId, operation] {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                tvManager->m_platformTVManagerClient->didTunerOperationChanged(tunerId, operation);
            });
        },
        // handle current source changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tuner_id.data, event->tuner_id.length);
            printf("\n%s:%s:%d Tuner ID Hello = %s\n", __FILE__, __func__, __LINE__,event->tuner_id.data);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, tunerId] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__,tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didCurrentSourceChanged(tunerId);
            });
        },
        // handle current channel changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            auto& tvManager = *reinterpret_cast<PlatformTVManager*>(data);
            tvManager.m_platformTVManagerClient->didCurrentChannelChanged(String(event->tuner_id.data, event->tuner_id.length));
        },
        // handle current source changed event
        [](void* data, wpe_tvcontrol_event* event)
        {

            uint16_t state = (uint16_t)event->state;
            String tunerId(event->tuner_id.data, event->tuner_id.length);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            RefPtr<PlatformTVChannel> channel = nullptr;

            if (event->channel_info) {
                tvManager->m_tvBackend->m_channel = event->channel_info;
                channel = PlatformTVChannel::create(tvManager->m_tvBackend, tunerId);
            }
            printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, event->tuner_id.data);
            callOnMainThread([tvManager, tunerId, state, channel] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didScanningStateChanged(tunerId, channel, state);
            });
        },

    };
    wpe_tvcontrol_backend_set_manager_event_client(m_tvBackend->m_backend, &s_eventClient, this);
}

PlatformTVManager::~PlatformTVManager()
{
    wpe_tvcontrol_backend_destroy(m_tvBackend->m_backend);
    delete m_tvBackend;
}

bool PlatformTVManager::getTuners(Vector<RefPtr<PlatformTVTuner>>& tunerVector)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);

    struct wpe_tvcontrol_string_vector tunerList;
    tunerList.length = 0;
    tvcontrol_return ret = TVControlFailed;
    ret = wpe_tvcontrol_backend_get_tuner_list(m_tvBackend->m_backend, &tunerList);
    if (ret == TVControlFailed || ret == TVControlNotImplemented)
        return false;
    if (tunerList.length) {
        for(uint64_t i = 0; i < tunerList.length; i++) {
            String tunerId(tunerList.strings[i].data, tunerList.strings[i].length);
            tunerVector.append(PlatformTVTuner::create(tunerId, m_tvBackend));
        }
    }
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return true;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
