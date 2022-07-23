/**
 * runtime.hpp - * runtime.cpp - Throwable runtime error
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

#include <stdexcept>
#include <string>

namespace vtrs{

class RuntimeError : public std::runtime_error {

private: // *** Private members *** //
    int m_code = 0;
    int m_kind = 0;

public: // *** Public members *** //

    /**
     * Runtime error types.
     *
     * A combination of error type and error kind can
     * determine the type and cause of an error.
     */
    enum ErrorKind : int {
        E_TYPE_GENERAL = 120,
        E_TYPE_VK_RESULT
    };

    /**
     * Defines a runtime exception.
     *
     * @param message The error message.
     * @param kind Error kind from {@link ErrorKind} enum.
     * @param code The error code, defaults to 0.
     */
    RuntimeError(const std::string&, int, int);
    RuntimeError(const std::string&, int);

    /**
     * Returns the error code.
     *
     * @return The error code.
    */
    [[nodiscard]] int getCode() const;

    /**
     * Returns the error kind.
     *
     * @return The error kind.
    */
    [[nodiscard]] int getKind() const;
};

} // namespace vtrs
