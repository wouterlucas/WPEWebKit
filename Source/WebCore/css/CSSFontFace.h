/*
 * Copyright (C) 2007, 2008, 2016 Apple Inc. All rights reserved.
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

#pragma once

#include "CSSFontFaceRule.h"
#include "FontTaggedSettings.h"
#include "TextFlags.h"
#include "Timer.h"
#include <memory>
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>

namespace JSC {
class ExecState;
}

namespace WebCore {

class CSSFontFaceSource;
class CSSFontSelector;
class CSSSegmentedFontFace;
class CSSValue;
class CSSValueList;
class Document;
class FontDescription;
class Font;
class FontFace;

class CSSFontFace final : public RefCounted<CSSFontFace> {
public:
    static Ref<CSSFontFace> create(CSSFontSelector* fontSelector, StyleRuleFontFace* cssConnection = nullptr, FontFace* wrapper = nullptr, bool isLocalFallback = false)
    {
        return adoptRef(*new CSSFontFace(fontSelector, cssConnection, wrapper, isLocalFallback));
    }
    virtual ~CSSFontFace();

    // FIXME: These functions don't need to have boolean return values.
    // Callers only call this with known-valid CSS values.
    bool setFamilies(CSSValue&);
    bool setStyle(CSSValue&);
    bool setWeight(CSSValue&);
    bool setUnicodeRange(CSSValue&);
    bool setVariantLigatures(CSSValue&);
    bool setVariantPosition(CSSValue&);
    bool setVariantCaps(CSSValue&);
    bool setVariantNumeric(CSSValue&);
    bool setVariantAlternates(CSSValue&);
    bool setVariantEastAsian(CSSValue&);
    void setFeatureSettings(CSSValue&);

    enum class Status;
    struct UnicodeRange;
    const CSSValueList* families() const { return m_families.get(); }
    FontTraitsMask traitsMask() const { return m_traitsMask; }
    const Vector<UnicodeRange>& ranges() const { return m_ranges; }
    const FontFeatureSettings& featureSettings() const { return m_featureSettings; }
    const FontVariantSettings& variantSettings() const { return m_variantSettings; }
    void setVariantSettings(const FontVariantSettings& variantSettings) { m_variantSettings = variantSettings; }
    void setTraitsMask(FontTraitsMask traitsMask) { m_traitsMask = traitsMask; }
    bool isLocalFallback() const { return m_isLocalFallback; }
    Status status() const { return m_status; }
    StyleRuleFontFace* cssConnection() const { return m_cssConnection.get(); }

    static Optional<FontTraitsMask> calculateStyleMask(CSSValue& style);
    static Optional<FontTraitsMask> calculateWeightMask(CSSValue& weight);

    class Client;
    void addClient(Client&);
    void removeClient(Client&);

    bool allSourcesFailed() const;

    void adoptSource(std::unique_ptr<CSSFontFaceSource>&&);
    void sourcesPopulated() { m_sourcesPopulated = true; }

    void fontLoaded(CSSFontFaceSource&);

    void load();
    RefPtr<Font> font(const FontDescription&, bool syntheticBold, bool syntheticItalic);

    static void appendSources(CSSFontFace&, CSSValueList&, Document*, bool isInitiatingElementInUserAgentShadowTree);

    class Client {
    public:
        virtual ~Client() { }
        virtual void fontLoaded(CSSFontFace&) { }
        virtual void fontStateChanged(CSSFontFace&, Status /*oldState*/, Status /*newState*/) { }
        virtual void fontPropertyChanged(CSSFontFace&, CSSValueList* /*oldFamilies*/ = nullptr) { }
        virtual void ref() = 0;
        virtual void deref() = 0;
    };

    // Pending => Loading  => TimedOut
    //              ||  \\    //  ||
    //              ||   \\  //   ||
    //              ||    \\//    ||
    //              ||     //     ||
    //              ||    //\\    ||
    //              ||   //  \\   ||
    //              \/  \/    \/  \/
    //             Success    Failure
    enum class Status { Pending, Loading, TimedOut, Success, Failure };

    struct UnicodeRange {
        UChar32 from;
        UChar32 to;
    };

    bool rangesMatchCodePoint(UChar32) const;

    // We don't guarantee that the FontFace wrapper will be the same every time you ask for it.
    Ref<FontFace> wrapper();
    void setWrapper(FontFace&);
    FontFace* existingWrapper() { return m_wrapper.get(); }

    bool webFontsShouldAlwaysFallBack() const;

    bool purgeable() const;

    void updateStyleIfNeeded();

#if ENABLE(SVG_FONTS)
    bool hasSVGFontFaceSource() const;
#endif

private:
    CSSFontFace(CSSFontSelector*, StyleRuleFontFace*, FontFace*, bool isLocalFallback);

    size_t pump();
    void setStatus(Status);
    void notifyClientsOfFontPropertyChange();

    void initializeWrapper();

    void fontLoadEventOccurred();
    void timeoutFired();

    RefPtr<CSSValueList> m_families;
    FontTraitsMask m_traitsMask { static_cast<FontTraitsMask>(FontStyleNormalMask | FontWeight400Mask) };
    Vector<UnicodeRange> m_ranges;
    FontFeatureSettings m_featureSettings;
    FontVariantSettings m_variantSettings;
    Timer m_timeoutTimer;
    Vector<std::unique_ptr<CSSFontFaceSource>> m_sources;
    RefPtr<CSSFontSelector> m_fontSelector;
    RefPtr<StyleRuleFontFace> m_cssConnection;
    HashSet<Client*> m_clients;
    WeakPtr<FontFace> m_wrapper;
    Status m_status { Status::Pending };
    bool m_isLocalFallback { false };
    bool m_sourcesPopulated { false };
    bool m_mayBePurged { true };
};

}
