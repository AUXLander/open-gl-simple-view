#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <thread>
#include <chrono>

#include <thread>
#include <mutex>

#include "gl_draw.h"

struct window_context
{
    size_t d_index = 0;
    size_t l_index = 0;
    size_t projection = 0;

    explorer*  explorer;
};

template<class Trender>
int draw_thread(std::mutex& gl_guard, const filemodel& fm, Trender& renderer, explorer& ex)
{
    GLFWwindow* window = glfwCreateWindow(800, 800, "MCML Draw", NULL, NULL);

    SetWindowPos(glfwGetWin32Window(window), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    std::string wtitle = "MCML Draw";

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    {
        std::lock_guard lock{ gl_guard };

        /* Make the window's context current */
        glfwMakeContextCurrent(window);

        if (!gladLoadGL())
        {
            glfwTerminate();
            return -1;
        }
    }

    ex.init(800, 800);

    std::cout << "Render device: " << glGetString(GL_RENDERER) << '\n';
    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << '\n';

    const auto [size_x, size_y, size_z, size_l] = fm.matrix_view.properties().size();

    auto ctime = std::chrono::system_clock::now() + std::chrono::milliseconds(1000 / 60);

    window_context context;

    context.explorer = &ex;

    glfwSetWindowUserPointer(window, &context);

    glfwSetScrollCallback(
        window, 
        [](GLFWwindow* win, double xoffset, double yoffset)
        {
            auto context = static_cast<window_context*>(glfwGetWindowUserPointer(win));
            auto ex = static_cast<explorer*>(context->explorer);

            if (context->d_index == 0 && yoffset < 0)
            {
                return;
            }

            context->d_index += static_cast<int>(yoffset);

            ex->invalidate_frame();
        }
    );

    bool KEY_P_WAS_PRESSED = false;
    bool KEY_UP_WAS_PRESSED = false;
    bool KEY_DOWN_WAS_PRESSED = false;

    while (!glfwWindowShouldClose(window))
    {
        if (std::chrono::system_clock::now() >= ctime)
        {
            std::lock_guard lock{ gl_guard };

            glfwMakeContextCurrent(window);
            glfwPollEvents();

            renderer.set_depth_index(context.d_index);

            wtitle = Trender::name();

            if (context.projection & 0x1)
            {
                wtitle += " projection: ";
            }
            else
            {
                wtitle += ": z = ";
                wtitle += std::to_string(context.d_index % size_z);
            }

            wtitle += "; l = ";
            wtitle += std::to_string(context.l_index % size_l);

            SetWindowTextA(glfwGetWin32Window(window), wtitle.c_str());

            ctime = std::chrono::system_clock::now() + std::chrono::milliseconds(1000 / 60);

            ex.draw_texture(renderer, fm);

            /* Swap front and back buffers */
            glfwSwapBuffers(window);
        }

        if (KEY_P_WAS_PRESSED && glfwGetKey(window, GLFW_KEY_P) != GLFW_PRESS)
        {
            KEY_P_WAS_PRESSED = false;

            ex.invalidate_frame();

            renderer.set_enable_projection(++context.projection & 0x1);
        }
        
        if (KEY_UP_WAS_PRESSED && glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS)
        {
            KEY_UP_WAS_PRESSED = false;

            ex.invalidate_frame();

            renderer.set_layer_index(++context.l_index);
        }

        if (KEY_DOWN_WAS_PRESSED && glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_PRESS)
        {
            KEY_DOWN_WAS_PRESSED = false;

            ex.invalidate_frame();

            renderer.set_layer_index(--context.l_index);
        }

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        {
            KEY_P_WAS_PRESSED = true;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            KEY_UP_WAS_PRESSED = true;
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            KEY_DOWN_WAS_PRESSED = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60 / 2));
    }

    glfwTerminate();

    return 0;
}

int main(void)
{
    std::mutex gl_guard;

    render_xy renderer1;
    explorer ex1;

    render_xz renderer2;
    explorer ex2;

    const filemodel fm("D:\\snapshot-2022_10_15-21_42_18.bin");

    if (!glfwInit())
    {
        return -1;
    }

    std::thread window_xy(draw_thread<render_xy>, std::ref(gl_guard), std::cref(fm), std::ref(renderer1), std::ref(ex1));
    std::thread window_xz(draw_thread<render_xz>, std::ref(gl_guard), std::cref(fm), std::ref(renderer2), std::ref(ex2));

    window_xy.detach();
    window_xz.detach();

    while (true)
    {
        std::this_thread::yield();
    }

    return 0;
}