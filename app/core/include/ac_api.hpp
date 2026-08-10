// ac_api.hpp
#pragma once

#if defined(AC_BUILD_DLL)
    #define AC_API __declspec(dllexport)
#elif defined(AC_DIRECT_BUILD)
    #define AC_API
#else
    #define AC_API __declspec(dllimport)
#endif