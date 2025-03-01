/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "CSSPropertyNames.h"
#include "ExceptionOr.h"
#include "ScriptWrappable.h"
#include <wtf/Forward.h>

namespace WebCore {

class CSSProperty;
class CSSRule;
class CSSStyleSheet;
class CSSValue;
class MutableStyleProperties;
class StyleProperties;
class StyledElement;

class CSSStyleDeclaration : public ScriptWrappable {
    WTF_MAKE_NONCOPYABLE(CSSStyleDeclaration); WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~CSSStyleDeclaration() { }

    virtual void ref() = 0;
    virtual void deref() = 0;

    virtual StyledElement* parentElement() const { return nullptr; }
    virtual CSSRule* parentRule() const = 0;
    virtual String cssText() const = 0;
    virtual ExceptionOr<void> setCssText(const String&) = 0;
    virtual unsigned length() const = 0;
    virtual String item(unsigned index) const = 0;
    virtual RefPtr<CSSValue> getPropertyCSSValue(const String& propertyName) = 0;
    virtual String getPropertyValue(const String& propertyName) = 0;
    virtual String getPropertyPriority(const String& propertyName) = 0;
    virtual String getPropertyShorthand(const String& propertyName) = 0;
    virtual bool isPropertyImplicit(const String& propertyName) = 0;
    virtual ExceptionOr<void> setProperty(const String& propertyName, const String& value, const String& priority) = 0;
    virtual ExceptionOr<String> removeProperty(const String& propertyName) = 0;

    // CSSPropertyID versions of the CSSOM functions to support bindings and editing.
    // Use the non-virtual methods in the concrete subclasses when possible.
    // The CSSValue returned by this function should not be exposed to the web as it may be used by multiple documents at the same time.
    virtual RefPtr<CSSValue> getPropertyCSSValueInternal(CSSPropertyID) = 0;
    virtual String getPropertyValueInternal(CSSPropertyID) = 0;
    virtual ExceptionOr<bool> setPropertyInternal(CSSPropertyID, const String& value, bool important) = 0;

    virtual Ref<MutableStyleProperties> copyProperties() const = 0;

    virtual CSSStyleSheet* parentStyleSheet() const { return nullptr; }

protected:
    CSSStyleDeclaration() { }
};

} // namespace WebCore
