#include <cstdlib>

#include "platform/except.hpp"
#include "platform/logger.hpp"
#include "platform/linux/xcb_client.hpp"

#include "renderer/except.hpp"
#include "renderer/renderer_context.hpp"
#include "renderer/service_provider.hpp"


int testVitreousRenderer(const std::string& model_type, const std::string& texture_file, const std::string& model_file) {
    vtrs::XCBClient* xcb_client;
    vtrs::WindowSurface* surface;
    vtrs::XCBWindow window;
    vtrs::ServiceProvider* provider;
    vtrs::SurfacePresenter* presenter;

    vtrs::RendererGPU* rendererGPU = vtrs::RendererContext::getGPUList().front();

    for (auto next : vtrs::RendererContext::getGPUList()) {
        if (next->getScore() > rendererGPU->getScore()) {
            rendererGPU = next;
        }
    }

    rendererGPU->printInfo();

    try {
        xcb_client = new vtrs::XCBClient();
        window = xcb_client->createWindow(800, 600);
        surface = new vtrs::WindowSurface(xcb_client->getConnection(), &window);

        vtrs::ServiceProvider::Options service_options = {};
        service_options.queueFamilyIndices.insert(rendererGPU->getQueueFamilyIndex(vtrs::RendererGPU::QUEUE_FAMILY_INDEX_GRAPHICS));
        service_options.queueFamilyIndices.insert(rendererGPU->getQueueFamilyIndex(surface->getSurfaceHandle()));

        provider = vtrs::ServiceProvider::from(rendererGPU, &service_options);
        presenter = provider->createSurfacePresenter(surface);

    } catch (vtrs::RendererError& error) {
        vtrs::Logger::fatal(error.what());

        delete xcb_client;

        delete surface;
        delete presenter;
        delete provider;

        return EXIT_FAILURE;

    } catch (vtrs::PlatformError& error) {
        vtrs::Logger::fatal(error.what());
        return EXIT_FAILURE;
    }


    while (true) {
        auto event = xcb_client->pollEvents();

        if (event.kind == vtrs::WSIWindowEvent::WINDOW_EXPOSE) {
            continue;
        }

        if (event.kind == vtrs::WSIWindowEvent::KEY_PRESS && event.eventDetail == 24) {
            vtrs::Logger::info("User pressed Quit [Q] button!");
            break;
        }
    }

    delete presenter;
    delete provider;
    delete surface;
    delete xcb_client;

    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {

    if (argc <= 2) {
        vtrs::Logger::print("Usage: vulkan-test <model-type> <texture-file> [model-file]");
        return 0;
    }

    vtrs::RendererContext::initialise();

    std::string model_type = argv[1];
    std::string texture_file = argv[2];
    std::string model_file = argc >= 4 ? argv[3] : "";

    int status = testVitreousRenderer(model_type, texture_file, model_file);

    vtrs::RendererContext::destroy();
    return status;
}
