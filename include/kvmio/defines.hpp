
#pragma once
#include <kvmio/api_defines.hpp>
#if !defined(KVMIO_RELEASE) && !defined(KVMIO_DEBUG)
#   warning "None of KVMIO_RELEASE && KVMIO_DEBUG is defined; using KVMIO_DEBUG"
#   define KVMIO_DEBUG
#endif

