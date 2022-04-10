#include <glad/glad.h>
#include "ws_server.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <thread>

#include "gl_draw.h"

int main(void)
{
    std::thread server(start_server);

    //server.detach();

    GLFWwindow* window;
    int exit_code = 0;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(800, 800, "MCML Draw", NULL, NULL);

    SetWindowPos(glfwGetWin32Window(window), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (window)
    {
        /* Make the window's context current */
        glfwMakeContextCurrent(window);

        if (gladLoadGL())
        {
            std::cout << "Render device: " << glGetString(GL_RENDERER) << '\n';
            std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << '\n';


            explorer ex;

            ex.init(800, 800);

            ex.set_file("F:\\UserData\\Projects\\LightTransport\\mcml\\BINARY_DATA.BIN");

            ex.load_binary();

            ex.init(800, 800);

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