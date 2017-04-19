#include "config.h"
#include "TVManager.h"

#if ENABLE(TV_CONTROL)

#include "Document.h"
#include "Frame.h"
#include "Navigator.h"

namespace WebCore {

Ref<TVManager> TVManager::create(Navigator* navigator) {
    return adoptRef(*new TVManager(navigator));
}

TVManager::TVManager(Navigator* navigator)
   : ActiveDOMObject(navigator->frame()->document())
   , m_platformTVManager(nullptr) {
}

TVManager::~TVManager() {

}

const Vector<RefPtr<TVTuner>>&  TVManager::getTuners() {
    if (m_tunerList.size())
        return m_tunerList;

    if (!m_platformTVManager)
        m_platformTVManager = std::make_unique<PlatformTVManager>(this);

    // If the voiceList is empty, that's the cue to get the voices from the platform again.
    for (auto& tuner : m_platformTVManager->getTuners())
        m_tunerList.append(TVTuner::create(tuner));

    return m_tunerList;
}

ScriptExecutionContext* TVManager::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
