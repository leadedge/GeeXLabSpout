# GeexLabSpout
Spout sender dll for GeexLab 

http://www.geeks3d.com/geexlab/

to allow sharing the shader output with any Spout receiver.

Based on the dll example in he GeexLab sample pack : http://www.geeks3d.com/dl/showd/385

(\host_api\User_Plugin)

Each shader has to be edited individually to include the dll functions required.

Look at the example : GLSL_Spout\Spout_voronoi_distances.xml

SpoutControls.exe can be used to change the mouse and speed values.

This is a VS2012 project. For the project to compile, the Spout SDK source files have to be in a folder "SpoutSDK" one level above the project folder. SpoutControls.h and SpoutControls.cpp also have to be included. Get them here from Spout2 and ScpoutControls :

https://github.com/leadedge/Spout2/tree/master/SpoutSDK/Source

and

https://github.com/leadedge/SpoutControls/tree/master/SOURCE
