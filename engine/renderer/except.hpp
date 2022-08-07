/**
 * except.hpp - Throwable renderer exception
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2021-present Ajay Sreedhar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================================================================
 */

#pragma once

#include "except/runtime.hpp"

namespace vtrs {

class RendererError : public RuntimeError {

public:
    enum ErrorKind: int {
        E_TYPE_GENERAL = 311,
        E_TYPE_VK_RESULT
    };

    RendererError(const std::string& message, ErrorKind kind, int code) : RuntimeError(message, kind, code) {}
    RendererError(const std::string& message, ErrorKind kind) : RuntimeError(message, kind) {}
};

} // namespace vtrs
