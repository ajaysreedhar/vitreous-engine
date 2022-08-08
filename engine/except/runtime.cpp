/**
 * runtime.cpp - Throwable runtime error
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

#include "runtime.hpp"

vtrs::RuntimeError::RuntimeError(const std::string& message, int kind, int code = 0) : std::runtime_error(message) {
    this->m_code = code;
    this->m_kind = kind;
}

vtrs::RuntimeError::RuntimeError(const std::string& message, int kind): RuntimeError(message, kind, 0) {}

int vtrs::RuntimeError::getCode() const {
    return this->m_code;
}

int vtrs::RuntimeError::getKind() const {
    return this->m_kind;
}
