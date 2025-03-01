/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "JSDOMGlobalObject.h"
#include <wtf/Forward.h>

namespace WebCore {

class CSSValue;
class ScriptController;

typedef HashMap<void*, JSC::Weak<JSC::JSObject>> DOMObjectWrapperMap;

class DOMWrapperWorld : public RefCounted<DOMWrapperWorld> {
public:
    static Ref<DOMWrapperWorld> create(JSC::VM& vm, bool isNormal = false)
    {
        return adoptRef(*new DOMWrapperWorld(vm, isNormal));
    }
    WEBCORE_EXPORT ~DOMWrapperWorld();

    // Free as much memory held onto by this world as possible.
    WEBCORE_EXPORT void clearWrappers();

    void didCreateWindowShell(ScriptController* scriptController) { m_scriptControllersWithWindowShells.add(scriptController); }
    void didDestroyWindowShell(ScriptController* scriptController) { m_scriptControllersWithWindowShells.remove(scriptController); }

    void setShadowRootIsAlwaysOpen() { m_shadowRootIsAlwaysOpen = true; }
    bool shadowRootIsAlwaysOpen() const { return m_shadowRootIsAlwaysOpen; }

    // FIXME: can we make this private?
    DOMObjectWrapperMap m_wrappers;
    HashMap<CSSValue*, void*> m_cssValueRoots;

    bool isNormal() const { return m_isNormal; }

    JSC::VM& vm() const { return m_vm; }

protected:
    DOMWrapperWorld(JSC::VM&, bool isNormal);

private:
    JSC::VM& m_vm;
    HashSet<ScriptController*> m_scriptControllersWithWindowShells;
    bool m_isNormal;
    bool m_shadowRootIsAlwaysOpen { false };
};

DOMWrapperWorld& normalWorld(JSC::VM&);
WEBCORE_EXPORT DOMWrapperWorld& mainThreadNormalWorld();
inline DOMWrapperWorld& debuggerWorld() { return mainThreadNormalWorld(); }
inline DOMWrapperWorld& pluginWorld() { return mainThreadNormalWorld(); }

inline DOMWrapperWorld& currentWorld(JSC::ExecState* exec)
{
    return JSC::jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject())->world();
}

inline DOMWrapperWorld& worldForDOMObject(JSC::JSObject* object)
{
    return JSC::jsCast<JSDOMGlobalObject*>(object->globalObject())->world();
}
    
} // namespace WebCore
