//
//		GeeXLabSpout
//
//	Spout sender dll for GeexLab
//	
//	http://www.geeks3d.com/geexlab/
//	
//	to allow sharing the shader output with any Spout receiver.
//	
//	Based on the dll example in he GeexLab sample pack : http://www.geeks3d.com/dl/showd/385
//	(\host_api\User_Plugin)
//	
//	Each shader has to be edited individually to include the dll functions required.
//	Look at the example : GLSL_Spout\Spout_voronoi_distances.xml
//	
//	This is a VS2012 project. For the project to compile, the Spout SDK source files have to be
//	in a folder "SpoutSDK" one level above the project folder.
//	
//	https://github.com/leadedge/Spout2/tree/master/SpoutSDK/Source
//
//
//	----------------------------------------------------------------------------------
//	Revisons :
//
//		26.01.16	- updated GLSLHacker version for the new GeexLab 
//					- completed SpoutControls with speed
//		27.01.16	- cleaned up a basic version of the plugin
//					- Continued work on SpoutControls uses a separate experimental version
//					- Release local texture on stop
//		10.03.16	- branch testing
//		02.06.16	- rebuild for Spout 2.005
//		14.01.17	- rebuild for Spout 2.006
//		30.11.18	- rebuild for Spout 2.007 VS2017 - /MT
//		24.12.19	- rebuild x64 for Spout 2.007 VS2017 - /MT
//
//	----------------------------------------------------------------------------------
//
//		Copyright (C) 2016-2019. Lynn Jarvis http://spout.zeal.co/
//
//		This program is free software: you can redistribute it and/or modify
//		it under the terms of the GNU Lesser General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//
//		This program is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU Lesser General Public License for more details.
//
//		You will receive a copy of the GNU Lesser General Public License along 
//		with this program.  If not, see http://www.gnu.org/licenses/.
//
//	----------------------------------------------------------------------------------
//
#include "common.h"

// Spout
#include "..\..\SpoutSDK\Spout.h"


#ifdef _PLATFORM_WINDOWS
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#endif



struct RenderWindowData_v1
{
#ifdef _PLATFORM_WINDOWS
  HDC _dc; // Device context.
  HWND _hwnd; // Handle on the 3D window.
  HGLRC _glrc; // OpenGL context.
#endif

#ifdef _PLATFORM_OSX
  //CGLContextObj _glrc; // OpenGL context.
  void* _glrc; // OpenGL context.
#endif

#ifdef _PLATFORM_LINUX
  Display _display; 
  Window _window;
  //GLXContext _glrc; // OpenGL context.
  void* _glrc; // OpenGL context.
#endif
};


int g_width = 0;
int g_height = 0;
unsigned char* g_data = NULL;
size_t g_data_len = 0;
unsigned char* g_out_data = NULL;
size_t g_out_data_len = 0;
std::string g_message;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Spout
SpoutSender sender;			// The Spout sender object
char g_SenderName[256];		// The name of the sender - set by the user
GLuint g_SenderTexture = 0;	// Local OpenGL texture used for sharing
bool bInitialized = false;	// Initialization result

// Local function to create a texture that the sender can share
bool InitGLtexture(GLuint &texID, unsigned int width, unsigned int height)
{

	if(texID != 0) glDeleteTextures(1, &texID);

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int set_raw_data(unsigned char* data, size_t data_len)
{
  if (!data || !data_len)
    return 0;
  if (g_data_len != data_len)
  {
    if (g_data)
      delete [] g_data;
  }
  g_data_len = data_len;
  g_data = new unsigned char[data_len];
  memcpy(g_data, data, data_len);
  return 1;
}

int get_raw_data(unsigned char* data, size_t* data_len)
{
  if (!data_len)
    return 0;
  if (!data)
  {
    *data_len = g_data_len;
    return 1;
  }
  else
  {
    if (!g_out_data)
      return 0;
    size_t len = *data_len;
    if (g_out_data_len < len)
      len = g_out_data_len;
    memcpy(data, g_out_data, len);
    return 1;
  }
  return 0;
}

#ifdef __cplusplus
extern "C" {
#endif



#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API char* _PLUGIN_CALLING_CONVENTION gxl_dylib_get_message()
{
  return (char*)g_message.c_str();
}

#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_set_message(char* message)
{

  if (message) {
     g_message = std::string(message);
  }

  return 1;
}

#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_clear_message()
{
  g_message.clear();
  return 1;
}

#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_set_raw_data(unsigned char* data, size_t data_len)
{
	return set_raw_data(data, data_len);
}


#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_get_raw_data(unsigned char* data, size_t* data_len)
{
	return get_raw_data(data, data_len);
}


#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_clear_raw_data()
{
  if (g_data)
    delete [] g_data;
  g_data_len = 0;
  if (g_out_data)
    delete [] g_out_data;
  g_out_data_len = 0;
  return 1;
}

#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_start(void* render_window, int width, int height, const char* user_data, void* gxl_data)
{
	g_width = width;
	g_height = height;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout
	//
	// Create an OpenGL texture for data transfers
	InitGLtexture(g_SenderTexture, g_width, g_height);
	
	// Create a Spout sender
	if(g_message.empty()) g_message = "GeexLab"; // default sender name
	strcpy_s(g_SenderName, 256, (const char *)g_message.c_str());
	bInitialized = sender.CreateSender(g_SenderName, g_width, g_height);
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -


	return 1;
}


#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_stop(const char* user_data, void* gxl_data)
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout
	if(g_SenderTexture != 0) glDeleteTextures(1, &g_SenderTexture);
	if(bInitialized) sender.ReleaseSender();
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  return 0;
}


#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_frame(float elapsed_time, const char* user_data)
{
 	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout
	if(bInitialized) {

		// Grab the screen into the local spout texture
		glBindTexture(GL_TEXTURE_2D, g_SenderTexture); 
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_width, g_height);
		glBindTexture(GL_TEXTURE_2D, 0); 

		// Send the texture out
		sender.SendTexture(g_SenderTexture, GL_TEXTURE_2D, g_width, g_height);
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  return 1;
}


#ifdef _PLATFORM_OSX
GXL_PLUGIN_EXPORT_OSX
#endif
GXL_DYLIB_DEMO_API int _PLUGIN_CALLING_CONVENTION gxl_dylib_resize(int width, int height, const char* user_data)
{
	g_width = width;
	g_height = height;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout
	// Update the sender texture size to receive the new dimensions
	InitGLtexture(g_SenderTexture, g_width, g_height);
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  return 1;
}

#ifdef __cplusplus
}
#endif



