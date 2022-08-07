#include <vulkan/vulkan.h>
#include "except.hpp"

#define VTRS_ASSERT_VK_RESULT(result, message) \
if (result != VK_SUCCESS) { \
    throw vtrs::RuntimeError(message, vtrs::RuntimeError::E_TYPE_VK_RESULT, result); \
}
