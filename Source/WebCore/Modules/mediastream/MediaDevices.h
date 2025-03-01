/*
 * Copyright (C) 2015 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(MEDIA_STREAM)

#include "JSDOMPromise.h"
#include "MediaDeviceInfo.h"

namespace WebCore {

class Document;
class MediaConstraintsImpl;
class MediaStream;
class MediaTrackSupportedConstraints;

class MediaDevices : public ScriptWrappable, public RefCounted<MediaDevices>, public ContextDestructionObserver {
public:
    static Ref<MediaDevices> create(Document&);

    Document* document() const;

    using Promise = DOMPromise<MediaStream>;
    using EnumerateDevicesPromise = DOMPromise<MediaDeviceInfoVector>;

    ExceptionOr<void> getUserMedia(Ref<MediaConstraintsImpl>&& audioConstraints, Ref<MediaConstraintsImpl>&& videoConstraints, Promise&&) const;
    void enumerateDevices(EnumerateDevicesPromise&&) const;
    RefPtr<MediaTrackSupportedConstraints> getSupportedConstraints();

private:
    explicit MediaDevices(Document&);
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
