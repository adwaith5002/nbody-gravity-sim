#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// -------------------------------------------------------
// SHADERS
// These are small programs that run ON THE GPU, not the CPU
// Written in GLSL (GL Shading Language) - looks like C
// -------------------------------------------------------

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main()
    {
        gl_Position = vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(1.0, 0.5, 0.0, 1.0);
    }
)";

int main()
{
    // --- Init GLFW ---
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    // Tell GLFW we want OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // --- Create Window ---
    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle Test", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // --- Init GLEW ---
    // Must happen AFTER making the OpenGL context current
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    // -------------------------------------------------------
    // COMPILE THE VERTEX SHADER
    // -------------------------------------------------------
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error:\n" << infoLog << "\n";
    }

    // -------------------------------------------------------
    // COMPILE THE FRAGMENT SHADER
    // -------------------------------------------------------
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error:\n" << infoLog << "\n";
    }

    // -------------------------------------------------------
    // LINK BOTH SHADERS INTO A SHADER PROGRAM
    // -------------------------------------------------------
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << "\n";
    }

    // Individual shaders no longer needed once linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // -------------------------------------------------------
    // TRIANGLE DATA - 3 vertices, each with x,y,z
    // Coordinates are in "NDC": -1.0 to 1.0 on each axis
    // (0,0) is the center of the screen
    // -------------------------------------------------------
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f, -0.5f, 0.0f,   // bottom right
         0.0f,  0.5f, 0.0f    // top center
    };

    // -------------------------------------------------------
    // VAO and VBO
    // VBO = Vertex Buffer Object: a chunk of memory ON THE GPU
    //       that holds your vertex data
    // VAO = Vertex Array Object: remembers HOW to read that data
    //       (how many floats per vertex, what each means)
    // -------------------------------------------------------
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO first - it will record everything we do next
    glBindVertexArray(VAO);

    // Upload vertex data to GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Tell the GPU: attribute 0 = position, 3 floats, starts at offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind (good habit)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // -------------------------------------------------------
    // RENDER LOOP
    // Runs every frame until the window is closed
    // -------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Check for Q key to quit
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Clear screen to dark grey
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swap front/back buffers (double buffering - prevents flickering)
        glfwSwapBuffers(window);

        // Process keyboard/mouse events
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}