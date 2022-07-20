/**
 * logger.hpp - Vitreous Engine [platform]
 * ===------------------------------------------------------------------===
 *
 * Copyright (c) 2022 Ajay Sreedhar
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
 * ===------------------------------------------------------------------===
 */

#pragma once

#include <iostream>
#include <string>
#include <initializer_list>



namespace vtrs {

    class Logger {

    private:
        static bool verbose;

        template<typename T>
        static void printUnpacked_(int level, T&& message) {
            switch (level) {
                case FATAL:
                case ERROR:
                case WARN:
                    std::cerr << message << " ";
                    break;

                default:
                    std::cout << message << " ";
                    break;
            }
        }

    public:
        enum LogLevel : int {
            FATAL = 0,
            ERROR,
            WARN,
            INFO,
            DEBUG,
            TRACE
        };

        static void init();

        static void verboseOn();
        static void verboseOff();

        template<typename... T> static void fatal(T&& ...messages) {
            std::cerr << "[FATAL] ";
            (printUnpacked_(FATAL, messages), ...);
            std::cerr << std::endl;
        }

        template<typename... T> static void error(T&& ...messages) {
            std::cerr << "[ERROR] ";
            (printUnpacked_(ERROR, messages), ...);
            std::cerr << std::endl;
        }

        template<typename... T> static void warn(T&& ...messages) {
            std::cerr << "[WARN ] ";
            (printUnpacked_(WARN, messages), ...);
            std::cerr << std::endl;
        }

        template<typename... T> static void info(T&& ...messages) {
            std::cout << "[INFO ] ";
            (printUnpacked_(INFO, messages), ...);
            std::cout << std::endl;
        }

        template<typename... T> static void debug(T&& ...messages) {
            std::cout << "[DEBUG] ";
            (printUnpacked_(DEBUG, messages), ...);
            std::cout << std::endl;
        }

        template<typename... T> static void trace(T&& ...messages) {
            std::cout << "[TRACE] ";
            (printUnpacked_(TRACE, messages), ...);
            std::cout << std::endl;
        }
    };

} // namespace vtrs
