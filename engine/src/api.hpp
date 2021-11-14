#pragma once

#ifdef _WIN32
#define JELLY_API extern "C" __declspec(dllexport)
#else
#define JELLY_API
#endif