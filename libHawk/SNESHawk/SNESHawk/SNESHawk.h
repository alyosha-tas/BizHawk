#ifdef _WIN32
#define SNESHawk_EXPORT extern "C" __declspec(dllexport)
#elif __linux__
#define SNESHawk_EXPORT extern "C"
#endif
