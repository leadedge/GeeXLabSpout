//
//		GeexLabSpout
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
//	SpoutControls.exe can be used to change the mouse and speed values.
//	
//	This is a VS2012 project. For the project to compile, the Spout SDK source files have to be in a folder "SpoutSDK" one level above the project folder. SpoutControls.h and SpoutControls.cpp also have to be included. Get them here from Spout2 and ScpoutControls :
//	
//	https://github.com/leadedge/Spout2/tree/master/SpoutSDK/Source
//	and
//	https://github.com/leadedge/SpoutControls/tree/master/SOURCE
//
//
//	----------------------------------------------------------------------------------
//	Revisons :
//
//		26.01.16	- updated GLSLHacker version for the new GeexLab 
//					- completed SpoutControls with speed
//
//	----------------------------------------------------------------------------------
//
//		Copyright (C) 2016. Lynn Jarvis http://spout.zeal.co/
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
#include "..\..\SpoutSDK\SpoutControls.h"

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
float g_Xvalue = 0.0f;
float g_Yvalue = 0.0f;
float g_Speed  = 0.0f;
float g_UserSpeed = 0.0f;
vector<control> myControls; // Vector of controls to be used
double elapsedTime = 0;
double lastTime = 0;
double PCFreq = 0;
__int64 CounterStart = 0;
void StartCounter();
double GetCounter();
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -



// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Spout Controls
// Local control update function
void UpdateControlParameters()
{
	float speed = 0.0f;

	// Control names and types must match those in the controls definition file
	// Names are case sensitive
	for(unsigned int i=0; i<myControls.size(); i++) {
		if(myControls.at(i).name == "MouseX") {
			g_Xvalue = myControls.at(i).value;
		}
		if(myControls.at(i).name == "MouseY") {
			g_Yvalue = myControls.at(i).value;
		}
		if(myControls.at(i).name == "Speed") {
			g_UserSpeed = myControls.at(i).value;
		}
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Spout
SpoutSender sender;
SpoutControls spoutcontrols;

char sendername[256];
GLuint sendertexture = 0;	// Local OpenGL texture used for sharing
bool bInitialized = false;	// Initialization result

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

void StartCounter()
{
    LARGE_INTEGER li;
	// Find frequency
    QueryPerformanceFrequency(&li);
    PCFreq = double(li.QuadPart)/1000.0;
	// Second call needed
    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}

double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
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

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout Controls
	float vpdim[4]; // viewport dimensions
	float mousex, mousey, speed;
	glGetFloatv(GL_VIEWPORT, vpdim);
	mousex = g_Xvalue*vpdim[2];
	mousey = g_Yvalue*vpdim[3];
	speed = g_Speed;

	if(strstr(message, "MouseX") > 0) {
		g_message = to_string(mousex);
	}
	else if(strstr(message, "MouseY") > 0) {
		g_message = to_string(mousey);
	}
	else if(strstr(message, "Speed") > 0) {
		g_message = to_string(speed);
	}
	else {
	    g_message = std::string(message);
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

	/*
	// Debug console window so printf works
	FILE* pCout; // should really be freed on exit 
	AllocConsole();
	freopen_s(&pCout, "CONOUT$", "w", stdout); 
	printf("GeexLabSpout start\n");
	*/

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	g_width = width;
	g_height = height;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Spout

	// Create an OpenGL texture for data transfers
	InitGLtexture(sendertexture, g_width, g_height);
	
	// Create a Spout sender
	if(g_message.empty()) g_message = "GeexLab"; // default sender name
	strcpy_s(sendername, 256, (const char *)g_message.c_str()); // safety
	bInitialized = sender.CreateSender(sendername, g_width, g_height);

	// Create Spout Controls
	spoutcontrols.OpenControls(sendername);

	// Start the clock
	StartCounter();
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
	if(bInitialized) sender.ReleaseSender();
	spoutcontrols.CloseControls();
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

		// ???
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);

		// Grab the screen into the local spout texture
		glBindTexture(GL_TEXTURE_2D, sendertexture); 
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_width, g_height);
		glBindTexture(GL_TEXTURE_2D, 0); 

		// Send the texture out
		sender.SendTexture(sendertexture, GL_TEXTURE_2D, g_width, g_height);

		// Check for changes to the controls by the controller
		if(spoutcontrols.CheckControls(myControls)) {
			UpdateControlParameters();
		}

		// Update time
		lastTime = elapsedTime;
		elapsedTime = GetCounter()/1000.0; // In seconds - higher resolution than timeGetTime()
		if(g_UserSpeed > 0)
			g_Speed = g_Speed + (float)(elapsedTime-lastTime)*g_UserSpeed*2.0f; // increment scaled by user input 0.0 - 2.0
		else
			g_Speed = g_Speed + (float)(elapsedTime-lastTime);
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
	// Update the sender texture to receive the new dimensions
	InitGLtexture(sendertexture, g_width, g_height);
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  return 1;
}

#ifdef __cplusplus
}
#endif



