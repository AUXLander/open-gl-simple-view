#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <thread>
#include <chrono>

#include "gl_draw.h"

int zmain(void)
{
    explorer ex;

    GLFWwindow* window;
    int exit_code = 0;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(800, 800, "MCML Draw", NULL, NULL);

    // SetWindowPos(glfwGetWin32Window(window), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(glfwGetWin32Window(window), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (window)
    {
        /* Make the window's context current */
        glfwMakeContextCurrent(window);

        if (gladLoadGL())
        {
            std::cout << "Render device: " << glGetString(GL_RENDERER) << '\n';
            std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << '\n';

            ex.init(800, 800);

            ex.set_file("D:\\BINARY_DATA.BIN");

            ex.load_binary();

            glClearColor(1, 1, 1, 1);

            auto timepoint = std::chrono::system_clock::now() + std::chrono::milliseconds(200);

            size_t z_index = 148;

            //ex.selected_layers[0] = false;
            //ex.selected_layers[1] = false;
            //ex.selected_layers[2] = false;
            //ex.selected_layers[3] = false;
            //ex.selected_layers[4] = true;

            /* Loop until the user closes the window */
            while (!glfwWindowShouldClose(window))
            {
                /* Poll for and process events */
                glfwPollEvents();

                if (std::chrono::system_clock::now() >= timepoint)
                {
                    ex.invalidate_frame();

                    ex.select_current_z_index(z_index++);

                    timepoint = std::chrono::system_clock::now() + std::chrono::milliseconds(200);
                }

                ex.DrawTexture();

                /* Swap front and back buffers */
                glfwSwapBuffers(window);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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