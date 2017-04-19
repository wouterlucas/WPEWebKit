#include "config.h"
#include "PlatformTVChannel.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

RefPtr<PlatformTVChannel> PlatformTVChannel::create(PlatformTVControlBackend* tvBackend, String tunerId)
{
    return adoptRef(new PlatformTVChannel(tvBackend, tunerId));
}

PlatformTVChannel::PlatformTVChannel(PlatformTVControlBackend* tvBackend, String tunerId)
    : m_tunerId(tunerId)
    , m_PlatformTVChannelClient(nullptr)
    , m_tvBackend(tvBackend)
{
    m_networkId         = tvBackend->m_channel->networkId;
    m_transportStreamId = tvBackend->m_channel->transportSId;
    m_serviceId         = tvBackend->m_channel->serviceId;
    m_name              = tvBackend->m_channel->name;
    m_number            = (std::to_string(tvBackend->m_channel->number)).c_str();
    m_type              = (PlatformTVChannel::Type)tvBackend->m_channel->type;
}

PlatformTVChannel::~PlatformTVChannel()
{
}

void PlatformTVChannel::setChannelClient(PlatformTVChannelClient* client)
{
    m_PlatformTVChannelClient = client;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
