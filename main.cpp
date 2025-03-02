#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "myopengl.hpp"
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace myopengl;

static float Yaw = 0.0f; // Rotación horizontal (izquierda-derecha)
static float Pitch = 0.0f; // Rotación vertical (arriba-abajo)
float mouseSensitivity = 0.5f;


glm::vec3 wasd_Movement = { 0.0f, 0.0f, 0.0f };
int m_CurrentView = 0;

float movementSpeed = 5.0f; // Velocidad base de movimiento
float lastFrame = 0.0f;
float deltaTime = 0.0f; // Tiempo entre frames

// Modified vertices with texture coordinates
GLfloat vertices[] = {
    //  Positions           // Colors            // Texture coords
    // Back face
    -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,    0.0f, 1.0f,

    // Front face
    -0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,    0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 1.0f,    1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f
};

// Índices para los triángulos del cubo
GLuint indices[] = {
    0, 1, 2, 2, 3, 0,  // Cara trasera
    4, 5, 6, 6, 7, 4,  // Cara delantera
    0, 4, 7, 7, 3, 0,  // Izquierda
    1, 5, 6, 6, 2, 1,  // Derecha
    3, 2, 6, 6, 7, 3,  // Arriba
    0, 1, 5, 5, 4, 0   // Abajo
};

// Updated vertex shader to handle textures
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 Color;
out vec2 TexCoord;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    Color = aColor;
    TexCoord = aTexCoord;
})";

// Updated fragment shader to handle multiple textures
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 Color;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform float mixRatio1;
uniform float mixRatio2;
uniform float mixRatio3;
uniform bool useMultiTexture;
uniform bool useTexture;

void main() {
    if(useTexture) {
        if(useMultiTexture) {
            // Combinación de múltiples texturas
            vec4 tex1 = texture(texture1, TexCoord) * mixRatio1;
            vec4 tex2 = texture(texture2, TexCoord) * mixRatio2;
            vec4 tex3 = texture(texture3, TexCoord) * mixRatio3;
            
            // Normalización de los ratios para asegurar que la suma es 1.0
            float totalRatio = mixRatio1 + mixRatio2 + mixRatio3;
            if (totalRatio > 0.0) {
                tex1 *= (mixRatio1 / totalRatio);
                tex2 *= (mixRatio2 / totalRatio);
                tex3 *= (mixRatio3 / totalRatio);
            }
            
            FragColor = tex1 + tex2 + tex3;
        } else {
            // Uso de una sola textura
            FragColor = texture(texture1, TexCoord);
        }
    } else {
        // Sin textura, solo color
        FragColor = vec4(Color, 1.0);
    }
})";

// Function to load a texture from file
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Flip textures on load
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Failed to load texture at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Estructura para manejar la configuración de multitextura
struct MultiTextureConfig {
    bool useMultiTexture;
    int texIndex1;
    int texIndex2;
    int texIndex3;
    float mixRatio1;
    float mixRatio2;
    float mixRatio3;
};

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1400, 1200, "Main Screen", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    static double limitFPS = 1.0 / 60.0;

    double lastTime = glfwGetTime(), timer = lastTime;
    double deltaTime = 0, nowTime = 0;
    int frames = 0, updates = 0;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_DEPTH_TEST);  // Activar Z-Buffer

    // Crear VAO, VBO y EBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Crear y compilar los shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Load textures
    std::vector<unsigned int> textures;
    textures.push_back(loadTexture("textures/wood.jpg"));     // Texture 0
    textures.push_back(loadTexture("textures/metal.jpg"));    // Texture 1
    textures.push_back(loadTexture("textures/concrete.jpg")); // Texture 2
    textures.push_back(loadTexture("textures/grass.jpeg"));   // Texture 3
    textures.push_back(loadTexture("textures/stone.jpeg"));    // Texture 4

    glm::vec3 posiciones[12] = {
        // Posiciones de los cubos
        glm::vec3(2.0f, -2.0f, 0.0f),  // x+1
        glm::vec3(-2.0f, -2.0f, 0.0f), // x-1
        glm::vec3(0.0f, -2.0f, 2.0f),  // z+1
        glm::vec3(0.0f, -2.0f, -2.0f),  // z-1
        glm::vec3(0.0f, 4.0f, 0.0f), //z+4

        // Posiciones de los tubos
        glm::vec3(-20.0f, -0.5f, 0.0f),  // z=0
        glm::vec3(20.0f, -0.5f, 0.0f),  // z=0
        glm::vec3(0.0f, -0.5f, 20.0f),  // z=0
        glm::vec3(0.0f, -0.5f, -20.0f),  // z=0

        // Posiciones de los tubos extruded en x y en z
        glm::vec3(0.0f, -0.5f, 0.0f),  // z=0
        glm::vec3(0.0f, -0.5f, 0.0f),  // z=0

        // Posicion tubo largo principal
        glm::vec3(0.0f, 0.5f, 0.0f)  // z=0
    };

    // Texture index for each cube (initially set to use different textures)
    int cubeTextures[12] = { 0, 1, 2, 3, 4, 0, 1, 2, 3, 0, 1, 2 };
    bool useTextures[12] = { true, true, true, true, true, true, true, true, true, true, true, true };

    // Configuración de multitextura para cada objeto
    MultiTextureConfig multiTexConfigs[12];
    for (int i = 0; i < 12; i++) {
        multiTexConfigs[i] = {
            false,           // useMultiTexture
            i % 5,           // texIndex1 (textura principal)
            (i + 1) % 5,     // texIndex2 (segunda textura)
            (i + 2) % 5,     // texIndex3 (tercera textura)
            1.0f,            // mixRatio1
            0.0f,            // mixRatio2
            0.0f             // mixRatio3
        };
    }

    // Habilitar multitextura para los primeros 3 objetos
    multiTexConfigs[0].useMultiTexture = true;
    multiTexConfigs[1].useMultiTexture = true;
    multiTexConfigs[2].useMultiTexture = true;

    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame; // Calcular el tiempo transcurrido entre frames
        lastFrame = currentFrame;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glUseProgram(shaderProgram);

        // Controles de movimiento
        if (ImGui::IsKeyDown(ImGuiKey_W)) wasd_Movement.y -= movementSpeed * deltaTime;
        else if (ImGui::IsKeyDown(ImGuiKey_S))
            wasd_Movement.y += movementSpeed * deltaTime;
        else if (ImGui::IsKeyDown(ImGuiKey_A))
            wasd_Movement.x += movementSpeed * deltaTime;
        else if (ImGui::IsKeyDown(ImGuiKey_D))
            wasd_Movement.x -= movementSpeed * deltaTime;
        else if (io.MouseWheel > 0)
            wasd_Movement.z += 200.0f * deltaTime;
        else if (io.MouseWheel < 0)
            wasd_Movement.z -= 200.0f * deltaTime;
        // Capturar movimiento del mouse
        if (io.MouseDown[1]) // Botón derecho presionado
        {
            float deltaX = io.MouseDelta.y; // Movimiento en X
            float deltaY = io.MouseDelta.x; // Movimiento en Y

            Yaw += deltaX * deltaTime * mouseSensitivity;   // Rotación en Y (izquierda-derecha)
            Pitch += deltaY * deltaTime * mouseSensitivity; // Rotación en X (arriba-abajo)
        }

        glm::mat4 View = glm::mat4(1.0f);

        // First move back to create some distance for viewing
        View = glm::translate(View, glm::vec3(wasd_Movement.x, wasd_Movement.y, -18.0f + wasd_Movement.z));

        // Then apply the rotations around the scene center
        View = glm::rotate(View, Pitch, glm::vec3(0.0f, 1.0f, 0.0f)); // rotate around y-axis
        View = glm::rotate(View, Yaw, glm::vec3(1.0f, 0.0f, 0.0f));   // rotate around x-axis

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        float angle = (float)glfwGetTime() * 0.4f;

        glBindVertexArray(VAO);

        for (int i = 0; i < 12; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            if (i >= 5 && i < 9)
            {
                model = scale(glm::vec3(0.1f, 2.0f, 0.1f));
            }
            else if (i == 9)
            {
                model = scale(glm::vec3(4.0f, 0.1f, 0.1f));
            }
            else if (i == 10)
            {
                model = scale(glm::vec3(0.1f, 0.1f, 4.0f));
            }
            else if (i == 11)
            {
                model = scale(glm::vec3(0.1f, 4.0f, 0.1f));
            }

            // Trasladar el cubo al origen
            model = glm::translate(model, posiciones[i]);

            model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) * model;

            glm::mat4 transform = projection * View * model;

            GLuint transformLoc = glGetUniformLocation(shaderProgram, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

            // Configurar texturas y uniforms para el shader
            glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture3"), 2);
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTextures[i]);
            glUniform1i(glGetUniformLocation(shaderProgram, "useMultiTexture"), multiTexConfigs[i].useMultiTexture);

            if (multiTexConfigs[i].useMultiTexture && useTextures[i]) {
                // Configurar ratios de mezcla para multitextura
                glUniform1f(glGetUniformLocation(shaderProgram, "mixRatio1"), multiTexConfigs[i].mixRatio1);
                glUniform1f(glGetUniformLocation(shaderProgram, "mixRatio2"), multiTexConfigs[i].mixRatio2);
                glUniform1f(glGetUniformLocation(shaderProgram, "mixRatio3"), multiTexConfigs[i].mixRatio3);

                // Activar y vincular las texturas a usar
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures[multiTexConfigs[i].texIndex1]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textures[multiTexConfigs[i].texIndex2]);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textures[multiTexConfigs[i].texIndex3]);
            }
            else {
                // Uso de una sola textura (como estaba antes)
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textures[cubeTextures[i]]);
            }

            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::Button("Reset View")) {
                Yaw = 0.0f;
                Pitch = 0.0f;
                wasd_Movement = { 0.f, 0.f, 0.0f };
            }
            ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.1f, 2.0f);

            // Texture selection UI
            ImGui::Separator();
            ImGui::Text("Texture Settings:");

            const char* textureNames[] = { "Wood", "Metal", "Concrete", "Grass", "Stone" };

            for (int i = 0; i < 5; i++) { // Only show controls for the first 5 cubes
                char label[32];

                // Create a unique ID for each combo box
                ImGui::PushID(i);

                // Checkbox for enabling/disabling texture
                ImGui::Checkbox("Use Texture", &useTextures[i]);

                if (useTextures[i]) {
                    // Combo box for texture selection
                    if (ImGui::Combo("Texture", &cubeTextures[i], textureNames, IM_ARRAYSIZE(textureNames))) {
                        // Handle texture change if needed
                    }
                }

                ImGui::PopID();
                ImGui::Separator();
            }

            // Multitexture Settings UI
            ImGui::Separator();
            ImGui::Text("Multitexture Settings:");

            for (int i = 0; i < 3; i++) { // Mostrar controles solo para los 3 primeros objetos
                ImGui::PushID(i + 100); // ID único para evitar conflictos

                const char* cube_name = "cube_x";
                ImGui::Checkbox(cube_name, &multiTexConfigs[i].useMultiTexture);

                if (multiTexConfigs[i].useMultiTexture) {
                    // Selector de texturas
                    const char* textureNames[] = { "Wood", "Metal", "Concrete", "Grass", "Stone" };

                    ImGui::Combo("Primary Texture", &multiTexConfigs[i].texIndex1, textureNames, IM_ARRAYSIZE(textureNames));
                    ImGui::Combo("Secondary Texture", &multiTexConfigs[i].texIndex2, textureNames, IM_ARRAYSIZE(textureNames));
                    ImGui::Combo("Tertiary Texture", &multiTexConfigs[i].texIndex3, textureNames, IM_ARRAYSIZE(textureNames));

                    // Controles deslizantes para los ratios de mezcla
                    ImGui::SliderFloat("Primary Mix", &multiTexConfigs[i].mixRatio1, 0.0f, 1.0f);
                    ImGui::SliderFloat("Secondary Mix", &multiTexConfigs[i].mixRatio2, 0.0f, 1.0f);
                    ImGui::SliderFloat("Tertiary Mix", &multiTexConfigs[i].mixRatio3, 0.0f, 1.0f);

                    // Botones de presets para efectos específicos
                    if (ImGui::Button("Blend Equal")) {
                        multiTexConfigs[i].mixRatio1 = 0.33f;
                        multiTexConfigs[i].mixRatio2 = 0.33f;
                        multiTexConfigs[i].mixRatio3 = 0.33f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Primary Dominant")) {
                        multiTexConfigs[i].mixRatio1 = 0.7f;
                        multiTexConfigs[i].mixRatio2 = 0.2f;
                        multiTexConfigs[i].mixRatio3 = 0.1f;
                    }
                }

                ImGui::PopID();
                ImGui::Separator();
            }

            // Texto de instrucciones
            ImGui::Text("Camera Controls:");
            ImGui::BulletText("WASD - Move camera");
            ImGui::BulletText("Right Click - Rotate camera");
            ImGui::BulletText("Mouse Wheel - Zoom in/out");
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Delete textures
    for (unsigned int texture : textures) {
        glDeleteTextures(1, &texture);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}