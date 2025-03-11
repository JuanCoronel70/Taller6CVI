# How to Install

This is an OpenGL-based project that uses **GLEW (OpenGL Extension Wrangler Library)** to manage OpenGL functions that may not be available due to versioning. The project also utilizes **GLFW (Graphics Library Framework)** to handle window creation, user input, events, and OpenGL contexts.

1. Download the GLEW binaries from the official [website](https://glew.sourceforge.net/).
2. Download the pre-compiled 32-bit binaries for Windows (or the appropriate version for your OS) from the official [GLFW website](https://www.glfw.org/download.html).
3. Extract the zip files.
4. Install Visual Studio with the C++ development tools (if you haven't done so already).
5. Clone this repository into any folder on your device. Then, open the project solution (`OpenGLProject1.vcxproj`) in Visual Studio, which can be found in the main folder of the cloned repository.
6. Right-click on the solution project and select **Properties** (make sure that the Configuration is set to "All Configurations" and the Platform is set to "Active"). Also, delete the paths inherited from cloning the repository.
   1. Go to **C++** → **General** → **Additional Include Directories** → **Edit** → **Add Directories** → Add the `included` folder from the "glew" directory you downloaded earlier, and also add the `include` folder from the "glfw" directory. Finally, click **Apply**.
   2. Go to **Linker** → **General** → **Additional Library Directories** → **Edit** → **Add Library** → Add the `win32` folder found in `glew/lib/release/win32`, and also add the `lib-vc2022` (or the folder matching your Visual Studio version) found in the "glfw" folder. Finally, click **Apply**.
   3. *(THIS STEP IS NOT REQUIRED IN THIS REPOSITORY)* Go to **Linker** → **Input** → **Additional Dependencies** → **Edit** → Add the following libraries on separate lines: `opengl32.lib`, `glew32.lib`, `glfw3.lib` (make sure to check the names of these libraries in your GLEW and GLFW folders).
   4. *(THE DLL FILE IS ALREADY CLONED IN THIS REPOSITORY, SO THIS STEP IS NOT NECESSARY)* Copy the `glew32.dll` file, which can be found in `glew/bin/Release/Win32`, and place it in the folder where the main source files of your project are located (the C++ files and others). You can use your OS file explorer to locate the repository and the source code folder.

