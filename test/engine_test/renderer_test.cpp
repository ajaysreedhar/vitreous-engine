#include <cstdlib>

#include "platform/logger.hpp"
#include "renderer/renderer_context.hpp"

int main(int argc, char** argv) {
    if (argc <= 1) {
        vtrs::Logger::print("Usage: renderer-test <module>");
        return EXIT_SUCCESS;
    }

    vtrs::RendererContext::initialise();

    auto gpu_list = vtrs::RendererContext::getGPUList();

    for(auto& gpu : gpu_list) {
        gpu->printInfo();
    }

    vtrs::RendererContext::destroy();

    return EXIT_SUCCESS;
}
