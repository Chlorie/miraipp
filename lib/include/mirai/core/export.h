#pragma once

#if defined(_MSC_VER)
#   define MPP_API_IMPORT __declspec(dllimport)
#   define MPP_API_EXPORT __declspec(dllexport)
#   define MPP_SUPPRESS_EXPORT_WARNING __pragma(warning(push)) __pragma(warning(disable: 4251 4275))
#   define MPP_RESTORE_EXPORT_WARNING __pragma(warning(pop))
#elif defined(__GNUC__)
#   define MPP_API_IMPORT
#   define MPP_API_EXPORT __attribute__((visibility("default")))
#   define MPP_SUPPRESS_EXPORT_WARNING
#   define MPP_RESTORE_EXPORT_WARNING
#else
#   define MPP_API_IMPORT
#   define MPP_API_EXPORT
#   define MPP_SUPPRESS_EXPORT_WARNING
#   define MPP_RESTORE_EXPORT_WARNING
#endif

#if defined(MPP_BUILD_SHARED)
#   ifdef MPP_EXPORT_SHARED
#       define MPP_API MPP_API_EXPORT
#   else
#       define MPP_API MPP_API_IMPORT
#   endif
#else
#   define MPP_API
#endif
