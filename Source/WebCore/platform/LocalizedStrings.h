/*
 * Copyright (C) 2003, 2006, 2009, 2011, 2012, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2010 Igalia S.L
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

#ifndef LocalizedStrings_h
#define LocalizedStrings_h

#include <wtf/Forward.h>

namespace WebCore {

    class IntSize;
    
    String inputElementAltText();
    String resetButtonDefaultLabel();
    String searchableIndexIntroduction();
    String submitButtonDefaultLabel();
    String fileButtonChooseFileLabel();
    String fileButtonChooseMultipleFilesLabel();
    String fileButtonNoFileSelectedLabel();
    String fileButtonNoFilesSelectedLabel();
    String defaultDetailsSummaryText();

#if PLATFORM(COCOA)
    String copyImageUnknownFileLabel();
#endif

#if ENABLE(CONTEXT_MENUS)
    WEBCORE_EXPORT String contextMenuItemTagOpenLinkInNewWindow();
    String contextMenuItemTagDownloadLinkToDisk();
    String contextMenuItemTagCopyLinkToClipboard();
    String contextMenuItemTagOpenImageInNewWindow();
    String contextMenuItemTagDownloadImageToDisk();
    String contextMenuItemTagCopyImageToClipboard();
#if PLATFORM(GTK) || PLATFORM(EFL)
    String contextMenuItemTagCopyImageUrlToClipboard();
#endif
    String contextMenuItemTagOpenFrameInNewWindow();
    String contextMenuItemTagCopy();
    String contextMenuItemTagGoBack();
    String contextMenuItemTagGoForward();
    String contextMenuItemTagStop();
    String contextMenuItemTagReload();
    String contextMenuItemTagCut();
    String contextMenuItemTagPaste();
#if PLATFORM(GTK)
    String contextMenuItemTagDelete();
    String contextMenuItemTagInputMethods();
    String contextMenuItemTagUnicode();
    String contextMenuItemTagUnicodeInsertLRMMark();
    String contextMenuItemTagUnicodeInsertRLMMark();
    String contextMenuItemTagUnicodeInsertLREMark();
    String contextMenuItemTagUnicodeInsertRLEMark();
    String contextMenuItemTagUnicodeInsertLROMark();
    String contextMenuItemTagUnicodeInsertRLOMark();
    String contextMenuItemTagUnicodeInsertPDFMark();
    String contextMenuItemTagUnicodeInsertZWSMark();
    String contextMenuItemTagUnicodeInsertZWJMark();
    String contextMenuItemTagUnicodeInsertZWNJMark();
#endif
#if PLATFORM(GTK) || PLATFORM(EFL)
    String contextMenuItemTagSelectAll();
#endif
    String contextMenuItemTagNoGuessesFound();
    String contextMenuItemTagIgnoreSpelling();
    String contextMenuItemTagLearnSpelling();
    String contextMenuItemTagSearchWeb();
    String contextMenuItemTagLookUpInDictionary(const String& selectedString);
    WEBCORE_EXPORT String contextMenuItemTagOpenLink();
    WEBCORE_EXPORT String contextMenuItemTagIgnoreGrammar();
    WEBCORE_EXPORT String contextMenuItemTagSpellingMenu();
    WEBCORE_EXPORT String contextMenuItemTagShowSpellingPanel(bool show);
    WEBCORE_EXPORT String contextMenuItemTagCheckSpelling();
    WEBCORE_EXPORT String contextMenuItemTagCheckSpellingWhileTyping();
    WEBCORE_EXPORT String contextMenuItemTagCheckGrammarWithSpelling();
    WEBCORE_EXPORT String contextMenuItemTagFontMenu();
    WEBCORE_EXPORT String contextMenuItemTagBold();
    WEBCORE_EXPORT String contextMenuItemTagItalic();
    WEBCORE_EXPORT String contextMenuItemTagUnderline();
    WEBCORE_EXPORT String contextMenuItemTagOutline();
    WEBCORE_EXPORT String contextMenuItemTagWritingDirectionMenu();
    String contextMenuItemTagTextDirectionMenu();
    WEBCORE_EXPORT String contextMenuItemTagDefaultDirection();
    WEBCORE_EXPORT String contextMenuItemTagLeftToRight();
    WEBCORE_EXPORT String contextMenuItemTagRightToLeft();
#if PLATFORM(COCOA)
    String contextMenuItemTagSearchInSpotlight();
    WEBCORE_EXPORT String contextMenuItemTagShowFonts();
    WEBCORE_EXPORT String contextMenuItemTagStyles();
    WEBCORE_EXPORT String contextMenuItemTagShowColors();
    WEBCORE_EXPORT String contextMenuItemTagSpeechMenu();
    WEBCORE_EXPORT String contextMenuItemTagStartSpeaking();
    WEBCORE_EXPORT String contextMenuItemTagStopSpeaking();
    WEBCORE_EXPORT String contextMenuItemTagCorrectSpellingAutomatically();
    WEBCORE_EXPORT String contextMenuItemTagSubstitutionsMenu();
    WEBCORE_EXPORT String contextMenuItemTagShowSubstitutions(bool show);
    WEBCORE_EXPORT String contextMenuItemTagSmartCopyPaste();
    WEBCORE_EXPORT String contextMenuItemTagSmartQuotes();
    WEBCORE_EXPORT String contextMenuItemTagSmartDashes();
    WEBCORE_EXPORT String contextMenuItemTagSmartLinks();
    WEBCORE_EXPORT String contextMenuItemTagTextReplacement();
    WEBCORE_EXPORT String contextMenuItemTagTransformationsMenu();
    WEBCORE_EXPORT String contextMenuItemTagMakeUpperCase();
    WEBCORE_EXPORT String contextMenuItemTagMakeLowerCase();
    WEBCORE_EXPORT String contextMenuItemTagCapitalize();
    String contextMenuItemTagChangeBack(const String& replacedString);
#endif
    String contextMenuItemTagOpenVideoInNewWindow();
    String contextMenuItemTagOpenAudioInNewWindow();
    String contextMenuItemTagDownloadVideoToDisk();
    String contextMenuItemTagDownloadAudioToDisk();
    String contextMenuItemTagCopyVideoLinkToClipboard();
    String contextMenuItemTagCopyAudioLinkToClipboard();
    String contextMenuItemTagToggleMediaControls();
    String contextMenuItemTagShowMediaControls();
    String contextMenuItemTagHideMediaControls();
    String contextMenuItemTagToggleMediaLoop();
    String contextMenuItemTagEnterVideoFullscreen();
    String contextMenuItemTagExitVideoFullscreen();
#if PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)
    String contextMenuItemTagEnterVideoEnhancedFullscreen();
    String contextMenuItemTagExitVideoEnhancedFullscreen();
#endif
    String contextMenuItemTagMediaPlay();
    String contextMenuItemTagMediaPause();
    String contextMenuItemTagMediaMute();
    WEBCORE_EXPORT String contextMenuItemTagInspectElement();
#endif // ENABLE(CONTEXT_MENUS)

#if !PLATFORM(IOS)
    String searchMenuNoRecentSearchesText();
    String searchMenuRecentSearchesText();
    String searchMenuClearRecentSearchesText();
#endif

    String AXWebAreaText();
    String AXLinkText();
    String AXListMarkerText();
    String AXImageMapText();
    String AXHeadingText();
    String AXDefinitionText();
    String AXDescriptionListText();
    String AXDescriptionListTermText();
    String AXDescriptionListDetailText();
    String AXFooterRoleDescriptionText();
    String AXFileUploadButtonText();
    String AXOutputText();
    String AXSearchFieldCancelButtonText();
    String AXAttachmentRoleText();
    String AXDetailsText();
    String AXSummaryText();
    String AXFigureText();
    String AXEmailFieldText();
    String AXTelephoneFieldText();
    String AXURLFieldText();
    String AXDateFieldText();
    String AXTimeFieldText();
    
    String AXButtonActionVerb();
    String AXRadioButtonActionVerb();
    String AXTextFieldActionVerb();
    String AXCheckedCheckBoxActionVerb();
    String AXUncheckedCheckBoxActionVerb();
    String AXMenuListActionVerb();
    String AXMenuListPopupActionVerb();
    String AXLinkActionVerb();
    String AXListItemActionVerb();

#if ENABLE(INPUT_TYPE_WEEK)
    // weekFormatInLDML() returns week and year format in LDML, Unicode
    // technical standard 35, Locale Data Markup Language, e.g. "'Week' ww, yyyy"
    String weekFormatInLDML();
#endif
#if PLATFORM(COCOA)
    String AXARIAContentGroupText(const String& ariaType);
    String AXHorizontalRuleDescriptionText();
    String AXMarkText();
#if ENABLE(METER_ELEMENT)
    String AXMeterGaugeRegionOptimumText();
    String AXMeterGaugeRegionSuboptimalText();
    String AXMeterGaugeRegionLessGoodText();
#endif
#endif
    
    String AXAutoFillCredentialsLabel();
    String AXAutoFillContactsLabel();

    String missingPluginText();
    String crashedPluginText();
    String blockedPluginByContentSecurityPolicyText();
    String insecurePluginVersionText();

    String multipleFileUploadText(unsigned numberOfFiles);
    String unknownFileSizeText();

#if PLATFORM(WIN)
    String uploadFileText();
    String allFilesText();
#endif

#if PLATFORM(COCOA)
    WEBCORE_EXPORT String builtInPDFPluginName();
    WEBCORE_EXPORT String pdfDocumentTypeDescription();
    WEBCORE_EXPORT String postScriptDocumentTypeDescription();
    String keygenMenuItem512();
    String keygenMenuItem1024();
    String keygenMenuItem2048();
    String keygenKeychainItemName(const String& host);
#endif

#if PLATFORM(IOS)
    String htmlSelectMultipleItems(size_t num);
    String fileButtonChooseMediaFileLabel();
    String fileButtonChooseMultipleMediaFilesLabel();
    String fileButtonNoMediaFileSelectedLabel();
    String fileButtonNoMediaFilesSelectedLabel();
#endif

    String imageTitle(const String& filename, const IntSize& size);

    String mediaElementLoadingStateText();
    String mediaElementLiveBroadcastStateText();
    String localizedMediaControlElementString(const String&);
    String localizedMediaControlElementHelpText(const String&);
    String localizedMediaTimeDescription(float);

    String validationMessageValueMissingText();
    String validationMessageValueMissingForCheckboxText();
    String validationMessageValueMissingForFileText();
    String validationMessageValueMissingForMultipleFileText();
    String validationMessageValueMissingForRadioText();
    String validationMessageValueMissingForSelectText();
    String validationMessageTypeMismatchText();
    String validationMessageTypeMismatchForEmailText();
    String validationMessageTypeMismatchForMultipleEmailText();
    String validationMessageTypeMismatchForURLText();
    String validationMessagePatternMismatchText();
    String validationMessageTooShortText(int valueLength, int minLength);
    String validationMessageTooLongText(int valueLength, int maxLength);
    String validationMessageRangeUnderflowText(const String& minimum);
    String validationMessageRangeOverflowText(const String& maximum);
    String validationMessageStepMismatchText(const String& base, const String& step);
    String validationMessageBadInputForNumberText();
#if USE(SOUP)
    String unacceptableTLSCertificate();
#endif

    String clickToExitFullScreenText();

#if ENABLE(VIDEO_TRACK)
    String textTrackSubtitlesText();
    String textTrackOffMenuItemText();
    String textTrackAutomaticMenuItemText();
    String textTrackNoLabelText();
    String audioTrackNoLabelText();
#if PLATFORM(COCOA) || PLATFORM(WIN)
    String textTrackCountryAndLanguageMenuItemText(const String& title, const String& country, const String& language);
    String textTrackLanguageMenuItemText(const String& title, const String& language);
    String closedCaptionTrackMenuItemText(const String&);
    String sdhTrackMenuItemText(const String&);
    String easyReaderTrackMenuItemText(const String&);
    String forcedTrackMenuItemText(const String&);
    String audioDescriptionTrackSuffixText(const String&);
#endif
#endif

    String snapshottedPlugInLabelTitle();
    String snapshottedPlugInLabelSubtitle();

    WEBCORE_EXPORT String useBlockedPlugInContextMenuTitle();

#if ENABLE(SUBTLE_CRYPTO)
    String webCryptoMasterKeyKeychainLabel(const String& localizedApplicationName);
    String webCryptoMasterKeyKeychainComment();
#endif

#if PLATFORM(MAC)
    WEBCORE_EXPORT String insertListTypeNone();
    WEBCORE_EXPORT String insertListTypeBulleted();
    WEBCORE_EXPORT String insertListTypeBulletedAccessibilityTitle();
    WEBCORE_EXPORT String insertListTypeNumbered();
    WEBCORE_EXPORT String insertListTypeNumberedAccessibilityTitle();
    WEBCORE_EXPORT String exitFullScreenButtonAccessibilityTitle();
#endif

#define WEB_UI_STRING(string, description) WebCore::localizedString(string)
#define WEB_UI_STRING_KEY(string, key, description) WebCore::localizedString(key)

    WEBCORE_EXPORT String localizedString(const char* key);

} // namespace WebCore

#endif // LocalizedStrings_h
