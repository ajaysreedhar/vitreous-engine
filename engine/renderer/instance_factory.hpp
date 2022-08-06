/**
 * instance_factory.hpp - Vulkan instance factory for abstraction.
 * ------------------------------------------------------------------------
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
 * ========================================================================
 */

#pragma once

#include <vector>
#include <string>

namespace vtrs {

class InstanceFactory {

private: // *** Private members *** //
    static bool s_isInitialised;

    /**
     * @brief Holds supported instance extensions.
     */
    static std::vector<std::string> s_iExtensions;

    /**
     * @brief Holds supported GPU device extensions.
     */
    std::vector<std::string> m_gExtensions {};

    /**
     * Initialises Vulkan instance.
     *
     * This method will set the m_instance member variable.
     */
    void initVulkan_();

    InstanceFactory();

public: // *** Public members *** //
    static InstanceFactory* factory();
};

} // namespace vtrs
