/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSXMLHttpRequest.h"

#include "Blob.h"
#include "DOMFormData.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Event.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLDocument.h"
#include "InspectorInstrumentation.h"
#include "JSBlob.h"
#include "JSDOMFormData.h"
#include "JSDOMWindowCustom.h"
#include "JSDocument.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "XMLHttpRequest.h"
#include <interpreter/StackVisitor.h>
#include <runtime/ArrayBuffer.h>
#include <runtime/Error.h>
#include <runtime/JSArrayBuffer.h>
#include <runtime/JSArrayBufferView.h>
#include <runtime/JSONObject.h>

using namespace JSC;

namespace WebCore {

void JSXMLHttpRequest::visitAdditionalChildren(SlotVisitor& visitor)
{
    if (XMLHttpRequestUpload* upload = wrapped().optionalUpload())
        visitor.addOpaqueRoot(upload);

    if (Document* responseDocument = wrapped().optionalResponseXML())
        visitor.addOpaqueRoot(responseDocument);
}

class SendFunctor {
public:
    SendFunctor()
        : m_hasSkippedFirstFrame(false)
        , m_line(0)
        , m_column(0)
    {
    }

    unsigned line() const { return m_line; }
    unsigned column() const { return m_column; }
    String url() const { return m_url; }

    StackVisitor::Status operator()(StackVisitor& visitor) const
    {
        if (!m_hasSkippedFirstFrame) {
            m_hasSkippedFirstFrame = true;
            return StackVisitor::Continue;
        }

        unsigned line = 0;
        unsigned column = 0;
        visitor->computeLineAndColumn(line, column);
        m_line = line;
        m_column = column;
        m_url = visitor->sourceURL();
        return StackVisitor::Done;
    }

private:
    mutable bool m_hasSkippedFirstFrame;
    mutable unsigned m_line;
    mutable unsigned m_column;
    mutable String m_url;
};

JSValue JSXMLHttpRequest::send(ExecState& state)
{
    InspectorInstrumentation::willSendXMLHttpRequest(wrapped().scriptExecutionContext(), wrapped().url());

    JSValue value = state.argument(0);
    ExceptionOr<void> result;
    if (value.isUndefinedOrNull())
        result = wrapped().send();
    else if (value.inherits(JSDocument::info()))
        result = wrapped().send(*JSDocument::toWrapped(value));
    else if (value.inherits(JSBlob::info()))
        result = wrapped().send(*JSBlob::toWrapped(value));
    else if (value.inherits(JSDOMFormData::info()))
        result = wrapped().send(*JSDOMFormData::toWrapped(value));
    else if (RefPtr<ArrayBuffer> arrayBuffer = toUnsharedArrayBuffer(value))
        result = wrapped().send(*arrayBuffer);
    else if (RefPtr<ArrayBufferView> arrayBufferView = toUnsharedArrayBufferView(value))
        result = wrapped().send(*arrayBufferView);
    else {
        // FIXME: If toString raises an exception, should we exit before calling willSendXMLHttpRequest?
        // FIXME: If toString raises an exception, should we exit before calling send?
        result = wrapped().send(value.toString(&state)->value(&state));
    }

    // FIXME: This should probably use ShadowChicken so that we get the right frame even when it did a tail call.
    // https://bugs.webkit.org/show_bug.cgi?id=155688
    SendFunctor functor;
    state.iterate(functor);
    wrapped().setLastSendLineAndColumnNumber(functor.line(), functor.column());
    wrapped().setLastSendURL(functor.url());

    // FIXME: Is it correct to do this only after the paragraph code of code just above, or should we exit earlier?
    if (UNLIKELY(result.hasException())) {
        propagateException(state, result.releaseException());
        return { };
    }

    return jsUndefined();
}

JSValue JSXMLHttpRequest::responseText(ExecState& state) const
{
    auto result = wrapped().responseText();

    if (UNLIKELY(result.hasException())) {
        propagateException(state, result.releaseException());
        return { };
    }

    return jsOwnedStringOrNull(&state, result.releaseReturnValue());
}

JSValue JSXMLHttpRequest::retrieveResponse(ExecState& state)
{
    auto type = wrapped().responseType();

    switch (type) {
    case XMLHttpRequest::ResponseType::EmptyString:
    case XMLHttpRequest::ResponseType::Text:
        return responseText(state);
    default:
        break;
    }

    if (!wrapped().doneWithoutErrors())
        return jsNull();

    JSValue value;
    switch (type) {
    case XMLHttpRequest::ResponseType::EmptyString:
    case XMLHttpRequest::ResponseType::Text:
        ASSERT_NOT_REACHED();
        return jsUndefined();

    case XMLHttpRequest::ResponseType::Json:
        value = JSONParse(&state, wrapped().responseTextIgnoringResponseType());
        if (!value)
            value = jsNull();
        break;

    case XMLHttpRequest::ResponseType::Document: {
        auto document = wrapped().responseXML();
        ASSERT(!document.hasException());
        value = toJS(&state, globalObject(), document.releaseReturnValue());
        break;
    }

    case XMLHttpRequest::ResponseType::Blob:
        value = toJSNewlyCreated(&state, globalObject(), wrapped().createResponseBlob());
        break;

    case XMLHttpRequest::ResponseType::Arraybuffer:
        value = toJS(&state, globalObject(), wrapped().createResponseArrayBuffer());
        break;
    }

    wrapped().didCacheResponse();
    return value;
}

} // namespace WebCore
