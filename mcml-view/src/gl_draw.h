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
#include "CImg.h"
#include "mcml_types.h"

namespace CIMG = cimg_library;

struct pixel
{
    uint8_t r{ 255 };
    uint8_t g{ 255 };
    uint8_t b{ 255 };

    uint8_t a{ 255 };

    pixel() = default;

    pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
        r{r}, g{g}, b{b}, a{ a }
    {;}
};

class explorer
{
    constexpr static auto openmode = std::ios_base::binary | std::ios_base::in;

    struct fstream_deleter
    {
        void operator()(std::fstream* s)
        {
            assert(s);
            s->close();
            delete s;
        }
    };

    std::unique_ptr<std::fstream, fstream_deleter> __file;
    std::deque<std::vector<float>> __matrix;

    std::unique_ptr<pixel[]> __texture;
    GLuint                   __texture_id;

    std::unique_ptr<uint8_t[]> __texture_draw;

    // headers
    short num_layers = 0;
    long  num_photons = 0;

    double max_x = 0;
    double max_y = 0;
    double max_z = 0;

    double min_x = 0;
    double min_y = 0;
    double min_z = 0;

    size_t __dpi_x = 0;
    size_t __dpi_y = 0;
    size_t __dpi_z = 0;

    size_t selected_z_index = 0;

    bool is_frame_invalidated{ true };

public:

    uint8_t* get_texture()
    {
        return (uint8_t*)__texture_draw.get();
    }

    size_t get_texture_size()
    {
        return 54 + 3 * view_width * view_height;
    }

    int frame_width;
    int frame_height;

    int view_width;
    int view_height;

    std::vector<bool> selected_layers;

    std::vector<std::vector<float>> heat_gamma_correction_values;

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

    void select_current_z_index(size_t z_index)
    {
        selected_z_index = z_index % __dpi_z;
    }

    void set_file(const char* filename)
    {
        __file.reset(new std::fstream{ filename, openmode });
    }

    void load_binary()
    {
        assert(__file);

        //__file->seekg(0, __file->end);
        //const size_t length = __file->tellg();
        //__file->seekg(0, __file->beg);


        size_t total_count = 0;

        __file->read((char*)&num_layers, sizeof(num_layers));
        __file->read((char*)&num_photons, sizeof(num_photons));

        __file->read((char*)&__dpi_x, sizeof(__dpi_x));
        __file->read((char*)&__dpi_y, sizeof(__dpi_y));
        __file->read((char*)&__dpi_z, sizeof(__dpi_z));

        __file->read((char*)&min_x, sizeof(min_x));
        __file->read((char*)&min_y, sizeof(min_y));
        __file->read((char*)&min_z, sizeof(min_z));

        __file->read((char*)&max_x, sizeof(max_x));
        __file->read((char*)&max_y, sizeof(max_y));
        __file->read((char*)&max_z, sizeof(max_z));

        __file->read((char*)&total_count, sizeof(total_count));

        const size_t size_of_layer = __dpi_x * __dpi_y * __dpi_z;

        frame_width = __dpi_x;
        frame_height = __dpi_y;
        
        selected_layers.resize(num_layers, true);
        heat_gamma_correction_values.resize(num_layers);

        for (short layer_idx = 0; layer_idx < num_layers; ++layer_idx)
        {
            std::vector<float> data(size_of_layer, 0.f);
            
            size_t value = 0;
            size_t index = 0;

            for (size_t z_index = 0; z_index < __dpi_z; ++z_index)
            {
                // to represent data as 100% interval with 5% step + buffer cell
                std::set<float, std::greater<float>> top_values;

                heat_gamma_correction_values[layer_idx].reserve(__dpi_z);

                for (size_t y_index = 0; y_index < __dpi_y; ++y_index)
                {
                    for (size_t x_index = 0; x_index < __dpi_x; ++x_index, ++index)
                    {
                        __file->read((char*)&value, sizeof(value));

                        top_values.insert(data[index] = static_cast<float>(value));

                        if (top_values.size() > 20)
                        {
                            top_values.erase(std::prev(top_values.end(), 1));
                        }
                    }
                }

                float top = 1.0;
                size_t shift = 1U;

                if (top_values.size() > shift)
                {
                    top = *(std::next(top_values.begin(), shift));
                }

                heat_gamma_correction_values[layer_idx].push_back(top);
            }

            __matrix.emplace_back(std::move(data));
        }

        int t;
    }

    void load_texture()
    {
        const GLenum target = GL_TEXTURE_2D;
        const GLenum format = GL_RGBA;

        if (__texture)
        {
            glEnable(target);
            glGenTextures(1, &__texture_id);
            glBindTexture(target, __texture_id);

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            glTexImage2D(target, 0, GL_RGBA8, frame_width, frame_height, 0, format, GL_UNSIGNED_BYTE, __texture.get());

            glDisable(target);
        }
    }

    void render_frame()
    {
        size_t dots_per_x = 1U;
        size_t dots_per_y = __dpi_x * dots_per_x;
        size_t dots_per_z = __dpi_y * dots_per_y;
        size_t dots_per_l = __dpi_z * dots_per_z;


        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_POINTS);

        size_t z_shift;
        size_t y_shift;
        size_t x_shift;
        float  gamma_correction = 0;

        for (size_t layer_index = 0; layer_index < selected_layers.size(); ++layer_index)
        {
            for (auto v : heat_gamma_correction_values[layer_index])
            {
                gamma_correction = std::fmax(gamma_correction, v);
            }
        }

        gamma_correction = 1000;

        for (size_t layer_index = 0; layer_index < selected_layers.size(); ++layer_index)
        {
            // gamma_correction = heat_gamma_correction_values[layer_index][selected_z_index];

            if (selected_layers[layer_index])
            {
                z_shift = selected_z_index * dots_per_z;

                /////////////

                for (int y = 0; y < __dpi_y; ++y)
                {
                    y_shift = y * dots_per_y;

                    for (int x = 0; x < __dpi_x; ++x)
                    {
                        x_shift = x * dots_per_x;

                        const auto value = __matrix[layer_index][z_shift + y_shift + x_shift] / gamma_correction;

                        if (value < 0.33f)
                        {
                            const auto alpha = 0.0 + (value - 0.00f) / 0.33f;

                            glColor4f(1.0, 1.0, 0, alpha);
                        }
                        else if (value < 0.66f)
                        {
                            const auto g = 1.0 - (value - 0.33f) / 0.33f;

                            glColor4f(1.0, g, 0, 1.0);
                        }
                        else
                        {
                            const auto r = 1.0 - (value - 0.66f) / 0.33f;

                            glColor4f(r, 0.0, 0, 1.0);
                        }

                        glVertex2i(x, y);
                    }
                }

                /////////////
            }
        }

        glDisable(GL_BLEND);
        glEnd();

        // save texture
        if (!__texture)
        {
            __texture.reset(new pixel[frame_width * frame_height]);
        }

        if (!__texture_draw)
        {
            __texture_draw.reset(new uint8_t[54 + 3 * view_width * view_height]{ 0 });
        }

        glReadPixels(0, 0, frame_width, frame_height, GL_RGBA, GL_UNSIGNED_BYTE, __texture.get());

        if constexpr (true)
        {
            CIMG::CImg<uint8_t> image((const char*)__texture.get(), 4, frame_width, frame_height, 1);

            image.permute_axes("yzcx");
            image.resize(view_width, view_height, -100, -100, 1);

            if constexpr (false)
            {
                image.save("blurred.bmp");
            }

            uint8_t* header = &__texture_draw.get()[0x00];
            uint8_t* body = &__texture_draw.get()[0x36];

            const int _width = view_width;
            const int _height = view_height;

            const unsigned int buf_size = (3 * _width) * (int)_height;
            const unsigned int file_size = 54 + buf_size;

            header[0x00] = 'B';
            header[0x01] = 'M';

            header[0x02] = (file_size >> 0) & 0xFF;
            header[0x03] = (file_size >> 8) & 0xFF;
            header[0x04] = (file_size >> 16) & 0xFF;
            header[0x05] = (file_size >> 24) & 0xFF;

            header[0x0A] = 0x36;
            header[0x0E] = 0x28;

            header[0x12] = (_width >> 0)  & 0xFF;
            header[0x13] = (_width >> 8)  & 0xFF;
            header[0x14] = (_width >> 16) & 0xFF;
            header[0x15] = (_width >> 24) & 0xFF;

            header[0x16] = (_height >> 0)  & 0xFF;
            header[0x17] = (_height >> 8)  & 0xFF;
            header[0x18] = (_height >> 16) & 0xFF;
            header[0x19] = (_height >> 24) & 0xFF;

            header[0x1A] = 1;
            header[0x1B] = 0;
            header[0x1C] = 24;
            header[0x1D] = 0;

            header[0x22] = (buf_size >> 0)  & 0xFF;
            header[0x23] = (buf_size >> 8)  & 0xFF;
            header[0x24] = (buf_size >> 16) & 0xFF;
            header[0x25] = (buf_size >> 24) & 0xFF;

            header[0x27] = 0x1;
            header[0x2B] = 0x1;

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

            if constexpr (false)
            {
                std::fstream file("new.bmp", std::ios_base::binary | std::ios_base::out);
                file.write((const char*)__texture_draw.get(), 54 + 3 * view_width * view_height);
            }
        }
    }

    void DrawTexture()
    {
        if (is_frame_invalidated)
        {
            render_frame();
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