/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "CryptoKeyHMAC.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoAlgorithmHmacKeyParams.h"
#include "CryptoAlgorithmRegistry.h"
#include "CryptoKeyDataOctetSequence.h"
#include "JsonWebKey.h"
#include <wtf/text/Base64.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

CryptoKeyHMAC::CryptoKeyHMAC(const Vector<uint8_t>& key, CryptoAlgorithmIdentifier hash, bool extractable, CryptoKeyUsageBitmap usage)
    : CryptoKey(CryptoAlgorithmIdentifier::HMAC, CryptoKeyType::Secret, extractable, usage)
    , m_hash(hash)
    , m_key(key)
{
}

CryptoKeyHMAC::CryptoKeyHMAC(Vector<uint8_t>&& key, CryptoAlgorithmIdentifier hash, bool extractable, CryptoKeyUsageBitmap usage)
    : CryptoKey(CryptoAlgorithmIdentifier::HMAC, CryptoKeyType::Secret, extractable, usage)
    , m_hash(hash)
    , m_key(WTFMove(key))
{
}

CryptoKeyHMAC::~CryptoKeyHMAC()
{
}

RefPtr<CryptoKeyHMAC> CryptoKeyHMAC::generate(size_t lengthBits, CryptoAlgorithmIdentifier hash, bool extractable, CryptoKeyUsageBitmap usages)
{
    if (!lengthBits) {
        switch (hash) {
        case CryptoAlgorithmIdentifier::SHA_1:
        case CryptoAlgorithmIdentifier::SHA_224:
        case CryptoAlgorithmIdentifier::SHA_256:
            lengthBits = 512;
            break;
        case CryptoAlgorithmIdentifier::SHA_384:
        case CryptoAlgorithmIdentifier::SHA_512:
            lengthBits = 1024;
            break;
        default:
            return nullptr;
        }
    }
    // CommonHMAC only supports key length that is a multiple of 8. Therefore, here we are a little bit different
    // from the spec as of 11 December 2014: https://www.w3.org/TR/WebCryptoAPI/#hmac-operations
    if (lengthBits % 8)
        return nullptr;

    return adoptRef(new CryptoKeyHMAC(randomData(lengthBits / 8), hash, extractable, usages));
}

RefPtr<CryptoKeyHMAC> CryptoKeyHMAC::importRaw(size_t lengthBits, CryptoAlgorithmIdentifier hash, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    size_t length = keyData.size() * 8;
    if (!length)
        return nullptr;
    // CommonHMAC only supports key length that is a multiple of 8. Therefore, here we are a little bit different
    // from the spec as of 11 December 2014: https://www.w3.org/TR/WebCryptoAPI/#hmac-operations
    if (lengthBits && lengthBits != length)
        return nullptr;

    return adoptRef(new CryptoKeyHMAC(WTFMove(keyData), hash, extractable, usages));
}

RefPtr<CryptoKeyHMAC> CryptoKeyHMAC::importJwk(size_t lengthBits, CryptoAlgorithmIdentifier hash, JsonWebKey&& keyData, bool extractable, CryptoKeyUsageBitmap usages, CheckAlgCallback&& callback)
{
    if (keyData.kty != "oct")
        return nullptr;
    if (!keyData.k)
        return nullptr;
    Vector<uint8_t> octetSequence;
    if (!base64URLDecode(keyData.k.value(), octetSequence))
        return nullptr;
    if (!callback(hash, keyData.alg))
        return nullptr;
    if (usages && keyData.use && keyData.use.value() != "sig")
        return nullptr;
    if (keyData.usages && ((keyData.usages & usages) != usages))
        return nullptr;
    if (keyData.ext && !keyData.ext.value() && extractable)
        return nullptr;

    return CryptoKeyHMAC::importRaw(lengthBits, hash, WTFMove(octetSequence), extractable, usages);
}

std::unique_ptr<KeyAlgorithm> CryptoKeyHMAC::buildAlgorithm() const
{
    return std::make_unique<HmacKeyAlgorithm>(CryptoAlgorithmRegistry::singleton().name(algorithmIdentifier()),
        CryptoAlgorithmRegistry::singleton().name(m_hash), m_key.size() * 8);
}

std::unique_ptr<CryptoKeyData> CryptoKeyHMAC::exportData() const
{
    return std::make_unique<CryptoKeyDataOctetSequence>(m_key);
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
