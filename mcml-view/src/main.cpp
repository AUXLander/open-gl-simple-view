#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include "gl_draw.h"

int main(void)
{
    //std::thread server(start_server);

    //server.detach();

    GLFWwindow* window;
    int exit_code = 0;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);

    if (window)
    {
        /* Make the window's context current */
        glfwMakeContextCurrent(window);

        if (gladLoadGL())
        {
            std::cout << "Render device: " << glGetString(GL_RENDERER) << '\n';
            std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << '\n';


            explorer ex;

            ex.init();

            glClearColor(1, 1, 1, 1);

            /* Loop until the user closes the window */
            while (!glfwWindowShouldClose(window))
            {
                /* Render here */
                glClear(GL_COLOR_BUFFER_BIT);


                ex.DrawTexture();


                /* Swap front and back buffers */
                glfwSwapBuffers(window);

                /* Poll for and process events */
                glfwPollEvents();
            }
        }
        else {
            exit_code = -1;
        }
    }
    else {
        exit_code = -1;
    }

    glfwTerminate();

    return exit_code;
}