#include "engine/platform/logger.hpp"
#include "wl-client.hpp"

vtest::WLClient *vtest::WLClient::getInstance() {
    return new WLClient();
}

vtest::WLClient::WLClient() = default;
