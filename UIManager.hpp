#pragma once

#include <GLFW/glfw3.h>

// Clase para manejar la UI
class UIManager {
public:
    UIManager();
    void Init(GLFWwindow* window);
    void Render();
    static void MouseCallback(GLFWwindow* window, int button, int action, int mods);

private:
    float btnX, btnY, btnWidth, btnHeight;
    bool buttonPressed;
    GLFWwindow* window;

    bool IsMouseOverButton(float mouseX, float mouseY);
    void DrawButton();
};

