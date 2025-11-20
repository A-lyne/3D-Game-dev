#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "learnopengl/shader_s.h"

#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Global rotation state for the Mario cube
int g_rotationState = 0; // 0=left, 1=right, 2=up-down, 3=back to left

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 1: Mario Cube Animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    std::cout << "Window created successfully: " << SCR_WIDTH << "x" << SCR_HEIGHT << std::endl;
    
    // Make sure window is visible
    glfwShowWindow(window);
    
    // Set vsync
    glfwSwapInterval(1);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    // Enable depth testing for 3D cube
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    // ------------------------------------
    std::cout << "Loading shaders: transform.vs and transform.fs" << std::endl;
    Shader ourShader("transform.vs", "transform.fs");
    if (ourShader.ID == 0) {
        std::cout << "ERROR: Shader compilation failed!" << std::endl;
        return -1;
    }
    std::cout << "Shader compiled successfully. ID: " << ourShader.ID << std::endl;

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // Cube vertices: 6 faces * 2 triangles * 3 vertices = 36 vertices
    float vertices[] = {
        // positions          // texture coords
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        
        // Back face
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        
        // Left face
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // load and create a texture (using mario.png for all faces)
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    
    // Helper function to find resource path (similar to Assignment_3)
    auto findResourcePath = [](const std::string& relativePath) -> std::string {
        // Try paths in order: Debug dir, build dir, source dir
        std::vector<std::string> pathsToTry = {
            "../" + relativePath,  // From Debug directory (build/Assignment_1/Debug/)
            relativePath,  // Current directory (if running from build/Assignment_1/)
            "../../Assignment_1/" + relativePath,  // From project root
            "../../../Assignment_1/" + relativePath,  // From deeper in build tree
            "../../../../Assignment_1/" + relativePath  // From project root if running from Debug
        };
        
        std::cout << "Searching for: " << relativePath << std::endl;
        for (const auto& path : pathsToTry) {
            std::ifstream test(path, std::ios::binary);
            if (test.good()) {
                test.close();
                std::cout << "  Found at: " << path << std::endl;
                return path;
            }
            test.close();
        }
        std::cout << "  WARNING: Not found in any location!" << std::endl;
        // Return original path if none found (will show error later)
        return relativePath;
    };
    
    // Load mario.png using helper function to find correct path
    std::string marioPath = findResourcePath("resources/textures/mario.png");
    std::cout << "Attempting to load mario.png from: " << marioPath << std::endl;
    unsigned char *data = stbi_load(marioPath.c_str(), &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cout << "Failed to load mario.png, trying additional paths..." << std::endl;
        // Try additional paths
        std::vector<std::string> additionalPaths = {
            "build/Assignment_1/resources/textures/mario.png",
            "Assignment_1/resources/textures/mario.png",
            "../../Assignment_1/resources/textures/mario.png",
            "../../../Assignment_1/resources/textures/mario.png"
        };
        for (const auto& path : additionalPaths) {
            std::cout << "Trying: " << path << std::endl;
            data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                std::cout << "Successfully loaded from: " << path << std::endl;
                break;
            }
        }
        if (!data) {
            std::cout << "Failed to load mario.png from all paths, trying container.jpg..." << std::endl;
            std::string containerPath = findResourcePath("resources/textures/container.jpg");
            data = stbi_load(containerPath.c_str(), &width, &height, &nrChannels, 0);
        }
    }
    
    if (data)
    {
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB;
        if (nrChannels == 4) {
            format = GL_RGBA;
            internalFormat = GL_RGBA;
        } else if (nrChannels == 1) {
            format = GL_RED;
            internalFormat = GL_RED;
        }
            
        std::cout << "Texture loaded successfully: " << width << "x" << height << ", channels: " << nrChannels << std::endl;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Mario texture applied to all 6 faces of the cube!" << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture. Creating default colored texture." << std::endl;
        // Create a default colored texture (red to match Mario theme)
        unsigned char defaultTexture[3] = {255, 0, 0}; // Red color
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, defaultTexture);
    }
    if (data) stbi_image_free(data);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use(); 
    ourShader.setInt("texture1", 0);

    std::cout << "Starting render loop..." << std::endl;
    std::cout << "Click on the cube to change rotation direction!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        
        // bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Get current time for animations
        float time = (float)glfwGetTime();

        // Create projection matrix (perspective for 3D)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        // Create view matrix (camera looking at cube from slightly away and above)
        glm::mat4 view = glm::lookAt(
            glm::vec3(3.0f, 3.0f, 3.0f),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // Looking at center
            glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
        );
        
        // Create model matrix (transforms the cube)
        glm::mat4 model = glm::mat4(1.0f);
        float rotationSpeed = 2.0f;
        
        // Apply rotation based on state
        switch (g_rotationState) {
            case 0: // Left (counter-clockwise around Y-axis)
                model = glm::rotate(model, time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                break;
            case 1: // Right (clockwise around Y-axis)
                model = glm::rotate(model, -time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                break;
            case 2: // Up-Down (rotate on X-axis - flip up and down)
                model = glm::rotate(model, std::sin(time * rotationSpeed) * glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                break;
            case 3: // Back to left (same as state 0)
                model = glm::rotate(model, time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                break;
        }
        
        // Combine matrices: projection * view * model
        glm::mat4 transform = projection * view * model;
        
        // Set transform matrix in shader
        unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
        
        // Draw the cube (36 vertices = 6 faces * 2 triangles * 3 vertices)
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever mouse button is clicked
// ---------------------------------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Change rotation state: 0 (left) -> 1 (right) -> 2 (up-down) -> 0 (back to left) (cycle)
        g_rotationState++;
        if (g_rotationState > 2) {
            g_rotationState = 0; // Back to left after state 2
        }
        std::cout << "Cube clicked! Rotation: ";
        switch(g_rotationState) {
            case 0: std::cout << "Left (counter-clockwise)"; break;
            case 1: std::cout << "Right (clockwise)"; break;
            case 2: std::cout << "Up-Down (flipping)"; break;
        }
        std::cout << std::endl;
    }
}
