/*
 * Copyright (C) 2003, 2006, 2008, 2012, 2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformCookieJar_h
#define PlatformCookieJar_h

#include <chrono>
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class URL;
class NetworkStorageSession;
struct Cookie;

// FIXME: These should probably be NetworkStorageSession member functions.

WEBCORE_EXPORT String cookiesForDOM(const NetworkStorageSession&, const URL& firstParty, const URL&);
WEBCORE_EXPORT void setCookiesFromDOM(const NetworkStorageSession&, const URL& firstParty, const URL&, const String&);
WEBCORE_EXPORT bool cookiesEnabled(const NetworkStorageSession&, const URL& firstParty, const URL&);
WEBCORE_EXPORT String cookieRequestHeaderFieldValue(const NetworkStorageSession&, const URL& firstParty, const URL&);
WEBCORE_EXPORT bool getRawCookies(const NetworkStorageSession&, const URL& firstParty, const URL&, Vector<Cookie>&);
WEBCORE_EXPORT void deleteCookie(const NetworkStorageSession&, const URL&, const String&);
WEBCORE_EXPORT void addCookie(const NetworkStorageSession&, const URL&, const Cookie&);
WEBCORE_EXPORT void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames);
WEBCORE_EXPORT void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames);
WEBCORE_EXPORT void deleteAllCookies(const NetworkStorageSession&);
WEBCORE_EXPORT void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point);
WEBCORE_EXPORT void setCookies(const NetworkStorageSession&, const Vector<Cookie>&);
WEBCORE_EXPORT bool getCookies(const NetworkStorageSession&, Vector<Cookie>&);

}

#endif
