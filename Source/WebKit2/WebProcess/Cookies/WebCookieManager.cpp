/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebCookieManager.h"

#include "ChildProcess.h"
#include "WebCookieManagerMessages.h"
#include "WebCookieManagerProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/CookieStorage.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/PlatformCookieJar.h>
#include <WebCore/URL.h>
#include <wtf/MainThread.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

const char* WebCookieManager::supplementName()
{
    return "WebCookieManager";
}

WebCookieManager::WebCookieManager(ChildProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebCookieManager::messageReceiverName(), *this);
}

void WebCookieManager::getHostnamesWithCookies(SessionID sessionID, uint64_t callbackID)
{
    HashSet<String> hostnames;
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::getHostnamesWithCookies(*storageSession, hostnames);

    Vector<String> hostnameList;
    copyToVector(hostnames, hostnameList);

    m_process->send(Messages::WebCookieManagerProxy::DidGetHostnamesWithCookies(hostnameList, callbackID), 0);
}

void WebCookieManager::deleteCookiesForHostname(SessionID sessionID, const String& hostname)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::deleteCookiesForHostnames(*storageSession, { hostname });
}

void WebCookieManager::deleteAllCookies(SessionID sessionID)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::deleteAllCookies(*storageSession);
}

void WebCookieManager::deleteAllCookiesModifiedSince(SessionID sessionID, std::chrono::system_clock::time_point time)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::deleteAllCookiesModifiedSince(*storageSession, time);
}

void WebCookieManager::addCookie(SessionID sessionID, const Cookie& cookie, const String& hostname)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::addCookie(*storageSession, URL(URL(), hostname), cookie);
}

void WebCookieManager::notifyCookiesDidChange(SessionID sessionID)
{
    ASSERT(RunLoop::isMain());
    m_process->send(Messages::WebCookieManagerProxy::CookiesDidChange(sessionID), 0);
}

void WebCookieManager::startObservingCookieChanges(SessionID sessionID)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID)) {
        WebCore::startObservingCookieChanges(*storageSession, [this, sessionID] {
            notifyCookiesDidChange(sessionID);
        });
    }
}

void WebCookieManager::stopObservingCookieChanges(SessionID sessionID)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::stopObservingCookieChanges(*storageSession);
}

void WebCookieManager::setHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy policy)
{
    platformSetHTTPCookieAcceptPolicy(policy);
}

void WebCookieManager::getHTTPCookieAcceptPolicy(uint64_t callbackID)
{
    m_process->send(Messages::WebCookieManagerProxy::DidGetHTTPCookieAcceptPolicy(platformGetHTTPCookieAcceptPolicy(), callbackID), 0);
}

void WebCookieManager::setCookies(SessionID sessionID, const Vector<WebCore::Cookie>& cookies)
{
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::setCookies(*storageSession, cookies);
}

void WebCookieManager::getCookies(SessionID sessionID, uint64_t callbackID)
{
    Vector<WebCore::Cookie> cookies;
    if (auto* storageSession = NetworkStorageSession::storageSession(sessionID))
        WebCore::getCookies(*storageSession, cookies);
    m_process->send(Messages::WebCookieManagerProxy::DidGetCookies(cookies, callbackID), 0);
}

} // namespace WebKit
