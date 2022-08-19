/**
 * ws_interface.hpp - Windowing System Interface abstraction.
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

#include <cstdint>

namespace vtrs {

struct wsi_window_event {
    enum EventKind: unsigned int {
        EMPTY_EVENT = 10,
        KEY_PRESS,
        BUTTON_PRESS,
        CLOSE_BUTTON_PRESS,
        WINDOW_EXPOSE
    };

    unsigned int kind = EMPTY_EVENT;

    uint32_t eventWindow = -468;
    uint32_t eventDetail = -468;

    unsigned int width = 0;
    unsigned int height = 0;
};

typedef struct wsi_window_event WSIWindowEvent;

} // namespace vtrs