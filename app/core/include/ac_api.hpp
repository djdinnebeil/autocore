// ac_api.hpp
#pragma once

#if defined(AC_BUILD_DLL)
    #define AC_API __declspec(dllexport)
#else
    #define AC_API __declspec(dllimport)
#endif