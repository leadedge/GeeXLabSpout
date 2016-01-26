
#ifndef _COMMON_H_
#define _COMMON_H_


#if defined(_WIN32) || defined(_WIN64)
  #define _PLATFORM_WINDOWS
  
  #include <windows.h>

  #include "GL/gl.h"
  #include "GL/glext.h"
  #include "GL/glu.h"
  #pragma comment(lib,"opengl32.lib")
  #pragma comment(lib,"glu32.lib")

  #ifdef GXL_DYLIB_DEMO_EXPORTS
    #define GXL_DYLIB_DEMO_API __declspec(dllexport)
  #else
    #define GXL_DYLIB_DEMO_API __declspec(dllimport)
  #endif
  #define _PLUGIN_CALLING_CONVENTION 

#elif defined(__APPLE__) || defined(__MACH__)
  #define _PLATFORM_OSX
  #define GXL_PLUGIN_EXPORT_OSX __attribute__((visibility("default")))
  #define GXL_DYLIB_DEMO_API
  #define _PLUGIN_CALLING_CONVENTION 

  #include <OpenGL/gl.h>
  #include <OpenGL/glext.h> 
  #include <OpenGL/OpenGL.h>
  #include <OpengL/glu.h>
  #include <OpenGL/CGLTypes.h>


#elif defined(__linux__) || defined(__unix__)
  #define GXL_DYLIB_DEMO_API
  #define _PLUGIN_CALLING_CONVENTION 

  #include <GL/glext.h>
  #include <GL/glx.h>
  #include <GL/glxext.h>

#endif


#include <string>
#include <vector>


#endif