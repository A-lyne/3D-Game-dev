#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "learnopengl/shader_s.h"

#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 1: 2D Animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("transform.vs", "transform.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // load and create a texture 
    // -------------------------
    unsigned int texture1, texture2;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    
    // Try to load mario.png first, fallback to container.jpg
    unsigned char *data = stbi_load("resources/textures/mario.png", &width, &height, &nrChannels, 0);
    if (!data)
    {
        // Fallback to container.jpg if mario.png doesn't exist
        data = stbi_load("resources/textures/container.jpg", &width, &height, &nrChannels, 0);
    }
    
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;
        else if (nrChannels == 1)
            format = GL_RED;
            
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture. Using default color." << std::endl;
    }
    stbi_image_free(data);
    
    // texture 2 - using a simple pattern or second texture
    // ---------
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Try to load awesomeface.png, or create a simple colored texture
    data = stbi_load("resources/textures/awesomeface.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        // Create a simple colored texture if awesomeface.png doesn't exist
        unsigned char simpleTexture[4] = {255, 200, 100, 255}; // Orange color
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, simpleTexture);
    }
    stbi_image_free(data);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use(); 
    ourShader.setInt("texture1", 0);
    ourShader.setInt("texture2", 1);

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
        glClear(GL_COLOR_BUFFER_BIT);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        ourShader.use();

        // Get current time for animations
        float time = (float)glfwGetTime();

        // Create multiple animated objects using sine/cosine functions
        // Object 1: Rotating and orbiting around center
        glm::mat4 transform1 = glm::mat4(1.0f);
        float orbitRadius = 0.4f;
        float orbitSpeed = 0.5f;
        transform1 = glm::translate(transform1, glm::vec3(
            std::cos(time * orbitSpeed) * orbitRadius,
            std::sin(time * orbitSpeed) * orbitRadius,
            0.0f
        ));
        transform1 = glm::rotate(transform1, time * 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        transform1 = glm::scale(transform1, glm::vec3(0.3f + 0.1f * std::sin(time * 2.0f), 
                                                      0.3f + 0.1f * std::sin(time * 2.0f), 
                                                      1.0f));

        unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform1));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Object 2: Pulsing and rotating at different speed
        glm::mat4 transform2 = glm::mat4(1.0f);
        float orbitRadius2 = 0.3f;
        float orbitSpeed2 = -0.7f; // Opposite direction
        transform2 = glm::translate(transform2, glm::vec3(
            std::cos(time * orbitSpeed2) * orbitRadius2,
            std::sin(time * orbitSpeed2) * orbitRadius2,
            0.0f
        ));
        transform2 = glm::rotate(transform2, time * -1.5f, glm::vec3(0.0f, 0.0f, 1.0f));
        float pulseScale = 0.25f + 0.15f * std::sin(time * 3.0f);
        transform2 = glm::scale(transform2, glm::vec3(pulseScale, pulseScale, 1.0f));

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform2));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Object 3: Sine wave motion along X-axis with rotation
        glm::mat4 transform3 = glm::mat4(1.0f);
        transform3 = glm::translate(transform3, glm::vec3(
            0.0f,
            std::sin(time * 1.5f) * 0.4f,
            0.0f
        ));
        transform3 = glm::rotate(transform3, time * 1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        transform3 = glm::scale(transform3, glm::vec3(0.2f, 0.2f, 1.0f));

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform3));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Object 4: Figure-8 pattern (Lissajous curve)
        glm::mat4 transform4 = glm::mat4(1.0f);
        float lissajousSpeed = 0.8f;
        transform4 = glm::translate(transform4, glm::vec3(
            std::sin(time * lissajousSpeed) * 0.35f,
            std::sin(time * lissajousSpeed * 2.0f) * 0.35f,
            0.0f
        ));
        transform4 = glm::rotate(transform4, time * 2.5f, glm::vec3(0.0f, 0.0f, 1.0f));
        float scale4 = 0.2f + 0.1f * std::cos(time * 2.0f);
        transform4 = glm::scale(transform4, glm::vec3(scale4, scale4, 1.0f));

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform4));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Object 5: Center object with breathing effect
        glm::mat4 transform5 = glm::mat4(1.0f);
        transform5 = glm::translate(transform5, glm::vec3(0.0f, 0.0f, 0.0f));
        transform5 = glm::rotate(transform5, time * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
        float breathScale = 0.15f + 0.05f * std::sin(time * 1.0f);
        transform5 = glm::scale(transform5, glm::vec3(breathScale, breathScale, 1.0f));

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform5));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

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

