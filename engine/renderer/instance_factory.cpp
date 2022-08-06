#include "instance_factory.hpp"

bool vtrs::InstanceFactory::s_isInitialised = false;
std::vector<std::string> vtrs::InstanceFactory::s_iExtensions {};

void vtrs::InstanceFactory::initVulkan_() {

}

vtrs::InstanceFactory::InstanceFactory() {
    initVulkan_();
}

vtrs::InstanceFactory *vtrs::InstanceFactory::factory() {
    auto factory = new InstanceFactory();
    return factory;
}
