#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <fstream>
#include <cassert>
#include <vector>
#include <list>
#include <set>
#include <deque>
#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include "CImg.h"
#include "mcml_types.h"
#include "../dpcpp-mcml/src/common/iofile.hpp"
#include "../dpcpp-mcml/src/common/matrix.hpp"

namespace CIMG = cimg_library;

struct pixel_rgb
{
    uint8_t r{ 255 };
    uint8_t g{ 255 };
    uint8_t b{ 255 };

    pixel_rgb() = default;

    pixel_rgb(uint8_t r, uint8_t g, uint8_t b) :
        r{ r }, g{ g }, b{ b }
    {;}
};

struct pixel_rgba : public pixel_rgb
{
    uint8_t a{ 255 };

    pixel_rgba() = default;

    pixel_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
        pixel_rgb{ r, g, b}, a{a}
    {;}
};

using pixel = pixel_rgba;

struct model
{
    const raw_memory_matrix_view<float>& matrix_view;

    model(raw_memory_matrix_view<float>& matrix_view) :
        matrix_view{ matrix_view }
    {;}

    std::string gist(size_t layer_index, size_t resolution = 100) const
    {
        std::string        result;
        std::vector<float> gist(resolution, 0.f);

        float min = matrix_view.at(0, 0, 0, layer_index);
        float max = matrix_view.at(0, 0, 0, layer_index);

        for (size_t z = 0; z < matrix_view.properties.size_z(); ++z)
        {
            for (size_t y = 0; y < matrix_view.properties.size_y(); ++y)
            {
                for (size_t x = 0; x < matrix_view.properties.size_x(); ++x)
                {
                    min = std::min(min, matrix_view.at(x, y, z, layer_index));
                    max = std::max(max, matrix_view.at(x, y, z, layer_index));
                }
            }
        }

        const auto maxind = gist.size() - 1U;
        const auto step = (max - min) / maxind;

        for (size_t z = 0; z < matrix_view.properties.size_z(); ++z)
        {
            for (size_t y = 0; y < matrix_view.properties.size_y(); ++y)
            {
                for (size_t x = 0; x < matrix_view.properties.size_x(); ++x)
                {
                    const auto value = matrix_view.at(x, y, z, layer_index);
                    const auto index = static_cast<size_t>((value - min) / step);

                    if (index < gist.size())
                    {
                        gist[index] += 1;
                    }
                }
            }
        }

        auto format = 
            [](float v)
            {
                auto str = std::to_string(v);
                str = str.substr(0, str.find(".") + 3);
                
                return str;
            };

        result += "{\"min\":" + format(min) + ",\"max\":" + format(max) + ",\"data\":[";

        for (const auto v : gist)
        {
            result += format(v);
            result += ',';
        }

        result.back() = ']';
        result += '}';

        return result;
    }
};

class filemodel : public model
{
    memory_matrix_view<float> __matrix;

public:

    filemodel()
        : model{ __matrix }
    {;}

    filemodel(const char* path)
        : model{ __matrix }
    {
        assert(path);

        auto file = iofile::open(path, std::ios_base::in);

        iofile::import_file(__matrix, file);
    }

    void import_file(iofile::file_handler& file)
    {
        iofile::import_file(__matrix, file);
    }

    void export_file(iofile::file_handler& file) const
    {
        iofile::export_file(__matrix, file);
    }
};

class explorer
{
    constexpr static size_t BMP_HEADER_SIZE = 54U;

    std::unique_ptr<uint8_t[]> __texture_save;
    std::unique_ptr<pixel[]>   __texture_draw;
    GLuint                     __texture_id;

    size_t selected_z_index = 0;
    size_t selected_l_index = 0;

    bool is_frame_invalidated{ true };

    void load_texture()
    {
        const GLenum target = GL_TEXTURE_2D;
        const GLenum format = GL_RGBA;

        if (__texture_draw)
        {
            glEnable(target);
            glGenTextures(1, &__texture_id);
            glBindTexture(target, __texture_id);

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            glTexImage2D(target, 0, GL_RGBA8, frame_width, frame_height, 0, format, GL_UNSIGNED_BYTE, __texture_draw.get());

            glDisable(target);
        }
    }

    float lower_bound = 0.0;
    float upper_bound = 1.0;

    void set_color(float value)
    {
        float g = 0;
        float a = 0;

        if (value < lower_bound)
        {
            a = 0;
        }
        else if (value > upper_bound)
        {
            a = 1;
        }
        else
        {
            a = (value - lower_bound) / (upper_bound - lower_bound);
            g = (1.0f - a);
        }

        glColor4f(1.0, g, 0, a);
    }

public:

    int frame_width;
    int frame_height;

    int view_width;
    int view_height;

    uint8_t* get_texture()
    {
        return reinterpret_cast<uint8_t*>(__texture_save.get());
    }

    size_t get_texture_size() const noexcept
    {
        return BMP_HEADER_SIZE + sizeof(pixel_rgb) * view_width * view_height;
    }

    void init(int ViewWidth, int ViewHeight)
    {
        view_width = ViewWidth;
        view_height = ViewHeight;

        glShadeModel(GL_SMOOTH);
        glMatrixMode(GL_PROJECTION_MATRIX);
        glLoadIdentity();
        glOrtho(0, view_height, 0, view_width, -1, 1);
        glViewport(0, 0, view_height, view_width);
    }

    void invalidate_frame()
    {
        is_frame_invalidated = true;
    }

    void set_z_index(size_t index)
    {
        selected_z_index = index;
    }

    void set_l_index(size_t index)
    {
        selected_l_index = index;
    }

    void set_lower_bound(float lower)
    {
        lower_bound = lower;
    }
    void set_upper_bound(float upper)
    {
        upper_bound = upper;
    }

    void render_frame(const model& m)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_POINTS);

        size_t l_index = selected_l_index;
        size_t z_index = selected_z_index;
        size_t y_index = 0;
        size_t x_index = 0;

        frame_width = m.matrix_view.properties.size_x();
        frame_height = m.matrix_view.properties.size_y();

        // for (;l_index < m.matrix_view.properties.size_l(); ++l_index)
        {
            // for (; z_index < m.matrix_view.properties.size_z(); ++z_index)
            {
                for (y_index = 0; y_index < m.matrix_view.properties.size_y(); ++y_index)
                {
                    for (x_index = 0; x_index < m.matrix_view.properties.size_x(); ++x_index)
                    {
                        set_color(m.matrix_view.at(x_index, y_index, z_index, l_index));

                        glVertex2i(static_cast<int>(x_index), static_cast<int>(y_index));
                    }
                }
            }
        }

        glDisable(GL_BLEND);
        glEnd();

        // save texture
        if (!__texture_draw)
        {
            __texture_draw.reset(new pixel[frame_width * frame_height]);
        }

        glReadPixels(0, 0, frame_width, frame_height, GL_RGBA, GL_UNSIGNED_BYTE, __texture_draw.get());

        if constexpr (true)
        {
            if (!__texture_save)
            {
                __texture_save.reset(new uint8_t[get_texture_size()]{0});
            }

            CIMG::CImg<uint8_t> image((const char*)__texture_draw.get(), 4, frame_width, frame_height, 1);

            image.permute_axes("yzcx");
            image.resize(view_width, view_height, -100, -100, 1);

            if constexpr (false)
            {
                image.save("blurred.bmp");
            }

            uint8_t* header = &__texture_save.get()[0x00];
            uint8_t* body = &__texture_save.get()[0x36];

            const int _width = view_width;
            const int _height = view_height;

            const unsigned int buf_size = (3 * _width) * (int)_height;
            const unsigned int file_size = 54 + buf_size;

            header[0x00] = 'B';
            header[0x01] = 'M';

            header[0x02] = 0xFF & (file_size >> 0);
            header[0x03] = 0xFF & (file_size >> 8);
            header[0x04] = 0xFF & (file_size >> 16);
            header[0x05] = 0xFF & (file_size >> 24);

            header[0x0A] = 0xFF & 0x36;
            header[0x0E] = 0xFF & 0x28;

            header[0x12] = 0xFF & (_width >> 0);
            header[0x13] = 0xFF & (_width >> 8);
            header[0x14] = 0xFF & (_width >> 16);
            header[0x15] = 0xFF & (_width >> 24);

            header[0x16] = 0xFF & (_height >> 0);
            header[0x17] = 0xFF & (_height >> 8);
            header[0x18] = 0xFF & (_height >> 16);
            header[0x19] = 0xFF & (_height >> 24);

            header[0x1A] = 0xFF & 1;
            header[0x1B] = 0xFF & 0;
            header[0x1C] = 0xFF & 24;
            header[0x1D] = 0xFF & 0;

            header[0x22] = 0xFF & (buf_size >> 0);
            header[0x23] = 0xFF & (buf_size >> 8);
            header[0x24] = 0xFF & (buf_size >> 16);
            header[0x25] = 0xFF & (buf_size >> 24);

            header[0x27] = 0xFF & 0x1;
            header[0x2B] = 0xFF & 0x1;

            const uint8_t
                * ptr_r = image.data(0, _height - 1, 0, 0),
                * ptr_g = image.data(0, _height - 1, 0, 1),
                * ptr_b = image.data(0, _height - 1, 0, 2);

            for (int y = 0; y < _height; ++y)
            {
                for (int x = 0; x < _width; ++x)
                {
                    *(body++) = *(ptr_b++);
                    *(body++) = *(ptr_g++);
                    *(body++) = *(ptr_r++);
                }

                ptr_r -= 2 * _width;
                ptr_g -= 2 * _width;
                ptr_b -= 2 * _width;
            }

            if constexpr (true)
            {
                auto file = iofile::open("new.bmp", std::ios_base::out);

                pipe_utils::save_raw_data(*file, (const char*)__texture_save.get(), get_texture_size());
            }
        }
    }

    void DrawTexture(const model& m)
    {
        if (is_frame_invalidated)
        {
            render_frame(m);
            load_texture();

            is_frame_invalidated = false;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, __texture_id);
        glBegin(GL_QUADS);

        glColor3f(1, 1, 1);

        glTexCoord2f(0, 0);
        glVertex2i(0, 0);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2i(0, view_height);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2i(view_width, view_height);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2i(view_width, 0);

        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
};