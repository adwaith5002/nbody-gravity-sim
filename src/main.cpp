#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 model;

    out vec3 FragPos;
    out vec3 Normal;

    void main()
    {
        FragPos     = vec3(model * vec4(aPos, 1.0));
        Normal      = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    out vec4 FragColor;

    uniform vec4 color;
    uniform vec3 lightPos;
    uniform int  useLight;

    void main()
    {
        if (useLight == 1) {
            vec3 norm     = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff    = max(dot(norm, lightDir), 0.0);
            float ambient = 0.2;
            float bright  = ambient + diff * 0.8;
            FragColor = vec4(color.rgb * bright, color.a);
        } else {
            FragColor = color;
        }
    }
)";

// -------------------------------------------------------
// BODY
// -------------------------------------------------------
struct Body {
    glm::vec3 position;
    glm::vec3 velocity;
    float     mass;
    float     radius;
    glm::vec4 color;
    bool      active;
};

// -------------------------------------------------------
// CAMERA
// -------------------------------------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 80.0f, 120.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float yaw        = -90.0f;
float pitch      =   0.0f;
float lastX      = 400.0f;
float lastY      = 300.0f;
bool  firstMouse = true;
float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

// -------------------------------------------------------
// SIMULATION
// -------------------------------------------------------
std::vector<Body> bodies;
const float G       = 6.6743e-4f;
const float DENSITY = 1.0f;
bool  placing    = false;
int   placingIdx = -1;

// -------------------------------------------------------
// GRID SETTINGS
// These define the grid resolution and size
// More cols/rows = smoother deformation, more CPU work
// -------------------------------------------------------
const int GRID_HALF = 150;
const int GRID_STEP = 2;
const float GRID_K    = 50000.0f;
const float GRID_SOFT = 10.0f;
const float GRID_MAX  = 80.0f;

// -------------------------------------------------------
// HELPERS
// -------------------------------------------------------
float radiusFromMass(float mass, float density)
{
    return cbrt((3.0f * mass) / (4.0f * 3.14159265f * density));
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    int success; char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        std::cerr << "Shader error: " << infoLog << "\n";
    }
    return id;
}

unsigned int createProgram(const char* vertSrc, const char* fragSrc)
{
    unsigned int vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// -------------------------------------------------------
// DYNAMIC GRID BUILD
// Called every frame with current body positions
// gridCenter = where the grid is centered (follows COM)
// -------------------------------------------------------
std::vector<float> buildDeformedGrid(glm::vec3 gridCenter)
{
    std::vector<float> verts;

    for (int i = -GRID_HALF; i <= GRID_HALF; i += GRID_STEP)
    {
        for (int j = -GRID_HALF; j < GRID_HALF; j += GRID_STEP)
        {
            float wx0 = gridCenter.x + i;
            float wz0 = gridCenter.z + j;

            float wx1 = gridCenter.x + i;
            float wz1 = gridCenter.z + j + GRID_STEP;

            auto getHeight = [&](float x, float z)
            {
                float h = 0.0f;

                for (const Body& b : bodies)
                {
                    if (!b.active)
                        continue;

                    float dx = x - b.position.x;
                    float dy = -b.position.y;
                    float dz = z - b.position.z;

                    float d2 = dx * dx + dy * dy + dz * dz;

                    float depth =
                        3.0f * log10f(glm::max(b.mass, 1.0f));

                    float width =
                        300.0f +
                        0.02f * sqrtf(b.mass);

                    h -= depth * expf(-d2 / width);
                }

                return h;
            };

            float h0 = getHeight(wx0, wz0);
            float h1 = getHeight(wx1, wz1);

            verts.push_back(wx0);
            verts.push_back(h0);
            verts.push_back(wz0);

            verts.push_back(wx1);
            verts.push_back(h1);
            verts.push_back(wz1);
        }
    }

    for (int j = -GRID_HALF; j <= GRID_HALF; j += GRID_STEP)
    {
        for (int i = -GRID_HALF; i < GRID_HALF; i += GRID_STEP)
        {
            float wx0 = gridCenter.x + i;
            float wz0 = gridCenter.z + j;

            float wx1 = gridCenter.x + i + GRID_STEP;
            float wz1 = gridCenter.z + j;

            auto getHeight = [&](float x, float z)
            {
                float h = 0.0f;

                for (const Body& b : bodies)
                {
                    if (!b.active)
                        continue;

                    float dx = x - b.position.x;
                    float dy = -b.position.y;
                    float dz = z - b.position.z;

                    float d2 = dx * dx + dy * dy + dz * dz;

                    float depth =
                        3.0f * log10f(glm::max(b.mass, 1.0f));

                    float width =
                        300.0f +
                        0.02f * sqrtf(b.mass);

                    h -= depth * expf(-d2 / width);
                }

                return h;
            };

            float h0 = getHeight(wx0, wz0);
            float h1 = getHeight(wx1, wz1);

            verts.push_back(wx0);
            verts.push_back(h0);
            verts.push_back(wz0);

            verts.push_back(wx1);
            verts.push_back(h1);
            verts.push_back(wz1);
        }
    }

    return verts;
}

// -------------------------------------------------------
// CALLBACKS
// -------------------------------------------------------
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = (xpos - lastX) * 0.1f;
    float yoffset = (lastY - ypos) * 0.1f;
    lastX = xpos; lastY = ypos;
    yaw   += xoffset;
    pitch += yoffset;
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraPos += (float)yoffset * 5.0f * cameraFront;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (!placing) {
            Body b;
            b.position = cameraPos + cameraFront * 30.0f;
            b.velocity = glm::vec3(0.0f);
            b.mass = 10000.0f;
            b.radius   = radiusFromMass(b.mass, DENSITY);
            b.color    = glm::vec4(0.3f, 0.8f, 1.0f, 1.0f);
            b.active   = false;
            bodies.push_back(b);
            placingIdx = bodies.size() - 1;
            placing    = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (placing && placingIdx >= 0) {
            bodies[placingIdx].active = true;
            placing    = false;
            placingIdx = -1;
        }
    }
}

void processInput(GLFWwindow* window)
{
    float speed = 20.0f * deltaTime;
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (placing && placingIdx >= 0) {
        // Right mouse or M = grow mass
        bool grow =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        if (grow) {
            bodies[placingIdx].mass   *= 1.0f + 2.0f * deltaTime;
            bodies[placingIdx].radius  = radiusFromMass(
                bodies[placingIdx].mass, DENSITY);
        }

        // Arrow keys reposition placing body
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            bodies[placingIdx].position += cameraUp * speed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            bodies[placingIdx].position -= cameraUp * speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            bodies[placingIdx].position -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            bodies[placingIdx].position += right * speed;

        // Enter launches the body
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            bodies[placingIdx].active = true;
            placing    = false;
            placingIdx = -1;
        }
    }
}

// -------------------------------------------------------
// SPHERE MESH
// -------------------------------------------------------
std::vector<float> buildSphere(int stacks, int sectors,
                                std::vector<unsigned int>& indices)
{
    std::vector<float> verts;
    for (int i = 0; i <= stacks; i++) {
        float phi = -1.5707963f + 3.14159265f * (float)i / stacks;
        for (int j = 0; j <= sectors; j++) {
            float theta = 2.0f * 3.14159265f * (float)j / sectors;
            float x = cos(phi) * cos(theta);
            float y = sin(phi);
            float z = cos(phi) * sin(theta);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
            int r1 = i * (sectors + 1);
            int r2 = (i + 1) * (sectors + 1);
            indices.push_back(r1 + j);
            indices.push_back(r2 + j);
            indices.push_back(r1 + j + 1);
            indices.push_back(r1 + j + 1);
            indices.push_back(r2 + j);
            indices.push_back(r2 + j + 1);
        }
    }
    return verts;
}

// -------------------------------------------------------
// MAIN
// -------------------------------------------------------
int main()
{
    if (!glfwInit()) { std::cerr << "GLFW failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D_TEST", NULL, NULL);
    if (!window) { std::cerr << "Window failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window,   mouseCallback);
    glfwSetScrollCallback(window,      scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW failed\n"; return -1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shader = createProgram(vertexShaderSource, fragmentShaderSource);

    // --- Sphere mesh (built once, reused for all bodies) ---
    std::vector<unsigned int> sphereIndices;
    std::vector<float> sphereVerts = buildSphere(24, 24, sphereIndices);

    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER,
        sphereVerts.size() * sizeof(float),
        sphereVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        sphereIndices.size() * sizeof(unsigned int),
        sphereIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    int sphereIndexCount = sphereIndices.size();

    // --- Grid mesh (dynamic — rebuilt every frame) ---
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

    // GL_DYNAMIC_DRAW tells the GPU: this buffer changes frequently
    // Allocate space but don't fill it yet (nullptr)
    // Size estimate: 2 directions * (grid lines) * 2 verts * 3 floats
    int gridLines   = (2 * GRID_HALF / GRID_STEP + 1);
    int maxVerts    = gridLines * gridLines * 2 * 2;
    glBufferData(GL_ARRAY_BUFFER,
        maxVerts * 3 * sizeof(float),
        nullptr, GL_DYNAMIC_DRAW);

    // Grid vertices are just positions (x,y,z) — no normals needed
    // So stride is 3 floats, not 6
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Location 1 (normal) not used for grid — disable it
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 750000.0f);

    glm::vec3 lightPos = glm::vec3(100.0f, 200.0f, 100.0f);

    // -------------------------------------------------------
    // RENDER LOOP
    // -------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- Calculate center of mass ---
        // Grid follows the weighted average position of all bodies
        glm::vec3 com    = glm::vec3(0.0f);
        float totalMass  = 0.0f;
        for (const Body& b : bodies) {
            if (!b.active) continue;
            com       += b.position * b.mass;
            totalMass += b.mass;
        }
        if (totalMass > 0.0f) {
            com /= totalMass;
            com.y = 0.0f;
        }  // grid always stays at ground level, only X and Z follow bodies
        // com is now the center of mass in world space

        // --- Physics ---
        for (int i = 0; i < (int)bodies.size(); i++) {
            if (!bodies[i].active) continue;
            for (int j = i + 1; j < (int)bodies.size(); j++) {
                if (!bodies[j].active) continue;

                glm::vec3 diff = bodies[j].position - bodies[i].position;
                float dist     = glm::length(diff);
                if (dist < 0.001f) continue;

                float softening    = bodies[i].radius + bodies[j].radius;
                float distSoftened = glm::max(dist, softening);
                float forceMag     = G * bodies[i].mass * bodies[j].mass
                                     / (distSoftened * distSoftened);
                glm::vec3 forceDir = glm::normalize(diff);

                bodies[i].velocity += forceDir * (forceMag / bodies[i].mass) * deltaTime;
                bodies[j].velocity -= forceDir * (forceMag / bodies[j].mass) * deltaTime;
            }
        }

        for (int i = 0; i < (int)bodies.size(); i++) {
            if (!bodies[i].active) continue;
            bodies[i].position += bodies[i].velocity * deltaTime;

            for (int j = i + 1; j < (int)bodies.size(); j++) {
                if (!bodies[j].active) continue;
                glm::vec3 diff  = bodies[j].position - bodies[i].position;
                float dist      = glm::length(diff);
                float minDist   = bodies[i].radius + bodies[j].radius;
                if (dist < minDist && dist > 0.001f) {
                    glm::vec3 axis = glm::normalize(diff);
                    float vi = glm::dot(bodies[i].velocity, axis);
                    float vj = glm::dot(bodies[j].velocity, axis);
                    bodies[i].velocity -= axis * vi * 1.2f;
                    bodies[j].velocity -= axis * vj * 1.2f;
                }
            }
        }

        // --- Rebuild deformed grid and upload to GPU ---
        std::vector<float> gridVerts = buildDeformedGrid(com);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

        // glBufferSubData updates part of an existing buffer
        // faster than glBufferData which reallocates every time
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            gridVerts.size() * sizeof(float),
            gridVerts.data());

        // --- Render ---
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            cameraPos, cameraPos + cameraFront, cameraUp);

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shader, "lightPos"),
            1, glm::value_ptr(lightPos));

        // Draw grid
        glm::mat4 identity = glm::mat4(1.0f);
        glUniform1i(glGetUniformLocation(shader, "useLight"), 0);
        glUniform4f(glGetUniformLocation(shader, "color"),
            0.2f, 0.6f, 0.8f, 0.7f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"),
            1, GL_FALSE, glm::value_ptr(identity));
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVerts.size() / 3);

        // Draw bodies
        glUniform1i(glGetUniformLocation(shader, "useLight"), 1);
        glBindVertexArray(sphereVAO);
        for (int i = 0; i < (int)bodies.size(); i++) {
            Body& b = bodies[i];
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, b.position);
            model = glm::scale(model, glm::vec3(b.radius));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"),
                1, GL_FALSE, glm::value_ptr(model));
            glm::vec4 drawColor = b.color;
            if (!b.active) drawColor.a = 0.6f;
            glUniform4fv(glGetUniformLocation(shader, "color"),
                1, glm::value_ptr(drawColor));
            glDrawElements(GL_TRIANGLES, sphereIndexCount,
                GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}