/*
 * Copyright (C) 2011, 2013, 2016 Apple Inc. All rights reserved.
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

#ifndef WebCookieManager_h
#define WebCookieManager_h

#include "HTTPCookieAcceptPolicy.h"
#include "MessageReceiver.h"
#include "NetworkProcessSupplement.h"
#include "WebProcessSupplement.h"
#include <WebCore/SessionID.h>
#include <stdint.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

#if USE(SOUP)
#include "SoupCookiePersistentStorageType.h"
#endif

namespace WebCore {
struct Cookie;
}

namespace WebKit {

class ChildProcess;

class WebCookieManager : public WebProcessSupplement, public NetworkProcessSupplement, public IPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(WebCookieManager);
public:
    WebCookieManager(ChildProcess*);

    static const char* supplementName();

    void setHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy);
#if USE(SOUP)
    void setCookiePersistentStorage(const String& storagePath, uint32_t storageType);
#endif

    void notifyCookiesDidChange(WebCore::SessionID);

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;

    void getHostnamesWithCookies(WebCore::SessionID, uint64_t callbackID);
    void deleteCookiesForHostname(WebCore::SessionID, const String&);
    void deleteAllCookies(WebCore::SessionID);
    void deleteAllCookiesModifiedSince(WebCore::SessionID, std::chrono::system_clock::time_point);
    void addCookie(WebCore::SessionID, const WebCore::Cookie&, const String& hostname);

    void platformSetHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy);
    void getHTTPCookieAcceptPolicy(uint64_t callbackID);
    HTTPCookieAcceptPolicy platformGetHTTPCookieAcceptPolicy();

    void setCookies(WebCore::SessionID, const Vector<WebCore::Cookie>& cookies);
    void getCookies(WebCore::SessionID, uint64_t callbackID);

    void startObservingCookieChanges(WebCore::SessionID);
    void stopObservingCookieChanges(WebCore::SessionID);

    ChildProcess* m_process;
};

} // namespace WebKit

#endif // WebCookieManager_h
