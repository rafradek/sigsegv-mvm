// #include "version.h"

// /* non-MSVC: add a dependency on autogen.h to ensure this file is rebuilt */
// /* MSVC: custom build step will update the timestamp on this file instead */
// #if !defined _MSC_VER
// #endif


const char *GetBuildDate(void) { return __DATE__; }
const char *GetBuildTime(void) { return __TIME__; }
