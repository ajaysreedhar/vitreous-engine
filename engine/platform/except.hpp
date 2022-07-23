/**
 * except.cpp - Throwable platform exception
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

class PlatformError : public RuntimeError {

public:
    PlatformError(std::string& message, ErrorKind kind, int code) : RuntimeError(message, kind, code) {}
    PlatformError(std::string& message, ErrorKind kind) : RuntimeError(message, kind){}
};

} // namespace vtrs
