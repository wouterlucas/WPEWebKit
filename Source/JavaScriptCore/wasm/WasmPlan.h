/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#if ENABLE(WEBASSEMBLY)

#include "CompilationResult.h"
#include "VM.h"
#include "WasmFormat.h"
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>

namespace JSC { namespace Wasm {
class Memory;

class Plan {
public:
    JS_EXPORT_PRIVATE Plan(VM*, Vector<uint8_t>);
    JS_EXPORT_PRIVATE Plan(VM*, const uint8_t*, size_t);
    JS_EXPORT_PRIVATE ~Plan();

    JS_EXPORT_PRIVATE void run();

    bool WARN_UNUSED_RETURN failed() const { return m_failed; }
    const String& errorMessage() const
    {
        RELEASE_ASSERT(failed());
        return m_errorMessage;
    }
    
    std::unique_ptr<ModuleInformation>& getModuleInformation()
    {
        RELEASE_ASSERT(!failed());
        return m_moduleInformation;
    }
    const Memory* memory() const
    {
        RELEASE_ASSERT(!failed());
        return m_moduleInformation->memory.get();
    }
    size_t compiledFunctionCount() const
    {
        RELEASE_ASSERT(!failed());
        return m_compiledFunctions.size();
    }
    const FunctionCompilation* compiledFunction(size_t i) const
    {
        RELEASE_ASSERT(!failed());
        return m_compiledFunctions.at(i).get();
    }
    CompiledFunctions& getCompiledFunctions()
    {
        RELEASE_ASSERT(!failed());
        return m_compiledFunctions;
    }

private:
    std::unique_ptr<ModuleInformation> m_moduleInformation;
    CompiledFunctions m_compiledFunctions;

    VM* m_vm;
    const uint8_t* m_source;
    const size_t m_sourceLength;
    bool m_failed { true };
    String m_errorMessage;
};

} } // namespace JSC::Wasm

#endif // ENABLE(WEBASSEMBLY)
