
#pragma once

#if (defined _WIN32 || defined __CYGWIN__) && defined(__GNUC__)
#	define KVMIO_IMPORT_API __declspec(dllimport)
#	define KVMIO_EXPORT_API __declspec(dllexport)
#else
#	define KVMIO_IMPORT_API __attribute__((visibility("default")))
#	define KVMIO_EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef KVMIO_BUILD_STATIC_LIBRARY
#	define KVMIO_API
#elif defined(KVMIO_BUILD_DYNAMIC_LIBRARY)
#	define KVMIO_API KVMIO_EXPORT_API
#elif defined(KVMIO_USE_DYNAMIC_LIBRARY)
#	define KVMIO_API KVMIO_IMPORT_API
#elif defined(KVMIO_USE_STATIC_LIBRARY)
#	define KVMIO_API
#else
#	define KVMIO_API
#endif

