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

struct model
{
    std::deque<std::deque<float>> __matrix;

    short num_layers = 0;
    long  num_photons = 0;

    double max_x = 0;
    double max_y = 0;
    double max_z = 0;

    double min_x = 0;
    double min_y = 0;
    double min_z = 0;

    // dots per line
    size_t dpl_x = 0;
    size_t dpl_y = 0;
    size_t dpl_z = 0;
    size_t dpl_l = 0;

    // dots per point

    size_t dpp_x = 0;
    size_t dpp_y = 0;
    size_t dpp_z = 0;
    size_t dpp_l = 0;

    std::string gist(size_t layer_idx, size_t resolution = 100) const
    {
        auto &matrix = __matrix[layer_idx];

        std::string        result;
        std::vector<float> data(resolution, 0.f);

        float min = static_cast<float>(matrix[0]);
        float max = static_cast<float>(matrix[0]);

        for (const auto v : matrix)
        {
            min = std::min(min, v);
            max = std::max(max, v);
        }

        //data.front() = count of max;
        //data.back()  = count of min;
        const auto maxind = data.size() - 1;
        const auto step = (max - min) / maxind;

        for (const auto v : matrix)
        {
            const auto index = static_cast<size_t>((v - min) / step);

            if (index < data.size())
            {
                data[index] += 1;
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

        for (const auto v : data)
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
    constexpr static auto readmode  = std::ios_base::binary | std::ios_base::in;
    constexpr static auto writemode = std::ios_base::binary | std::ios_base::out;

    struct fstream_deleter
    {
        void operator()(std::fstream* s)
        {
            assert(s);
            s->close();
            delete s;
        }
    };

    std::unique_ptr<std::fstream, fstream_deleter>  __file;

public:

    filemodel()
    {}

    filemodel(const char* filename)
    {
        //this->save(filename);
        this->load(filename);
    }

    void load(const char* filename)
    {
        __file.reset(new std::fstream{ filename, readmode });

        __file->read((char*)&num_layers, sizeof(num_layers));
        __file->read((char*)&num_photons, sizeof(num_photons));

        __file->read((char*)&dpl_x, sizeof(dpl_x));
        __file->read((char*)&dpl_y, sizeof(dpl_y));
        __file->read((char*)&dpl_z, sizeof(dpl_z));
        __file->read((char*)&dpl_l, sizeof(dpl_z));

        __file->read((char*)&min_x, sizeof(min_x));
        __file->read((char*)&min_y, sizeof(min_y));
        __file->read((char*)&min_z, sizeof(min_z));

        __file->read((char*)&max_x, sizeof(max_x));
        __file->read((char*)&max_y, sizeof(max_y));
        __file->read((char*)&max_z, sizeof(max_z));

        dpp_x = 1U;
        dpp_y = dpl_x * dpp_x;
        dpp_z = dpl_y * dpp_y;
        dpp_l = dpl_z * dpp_z;

        for (short layer_idx = 0; layer_idx < num_layers; ++layer_idx)
        {
            std::deque<float> data(dpp_l, 0.f);

            float value = 0;
            size_t index = 0;

            for (size_t z_index = 0; z_index < dpl_z; ++z_index)
            {
                for (size_t y_index = 0; y_index < dpl_y; ++y_index)
                {
                    for (size_t x_index = 0; x_index < dpl_x; ++x_index, ++index)
                    {
                        __file->read((char*)&value, sizeof(value));

                        data[index] = static_cast<float>(value);
                    }
                }
            }

            __matrix.emplace_back(std::move(data));
        }
    }

    void save(const char* filename)
    {
        __file.reset(new std::fstream{ filename, writemode });

        std::random_device rd;
        std::mt19937 gen{ rd() };
        std::normal_distribution<> ndist(0, 1.0);

        /// TEST DATA BEGIN

        num_layers = 5;
        num_photons = 102400;

        dpl_x = 100;
        dpl_y = 100;
        dpl_z = 100;
        dpl_l = num_layers;

        min_x = 0;
        min_y = 0;
        min_z = 0;

        max_x = 1;
        max_y = 1;
        max_z = 1;

        /// TEST DATA END

        __file->write((char*)&num_layers, sizeof(num_layers));
        __file->write((char*)&num_photons, sizeof(num_photons));

        __file->write((char*)&dpl_x, sizeof(dpl_x));
        __file->write((char*)&dpl_y, sizeof(dpl_y));
        __file->write((char*)&dpl_z, sizeof(dpl_z));
        __file->write((char*)&dpl_l, sizeof(dpl_z));

        __file->write((char*)&min_x, sizeof(min_x));
        __file->write((char*)&min_y, sizeof(min_y));
        __file->write((char*)&min_z, sizeof(min_z));

        __file->write((char*)&max_x, sizeof(max_x));
        __file->write((char*)&max_y, sizeof(max_y));
        __file->write((char*)&max_z, sizeof(max_z));

        for (short l_index = 0; l_index < num_layers; ++l_index)
        {
            //const std::deque<float>& data = __matrix[layer_idx];

            size_t index = 0;

            for (size_t z_index = 0; z_index < dpl_z; ++z_index)
            {
                for (size_t y_index = 0; y_index < dpl_y; ++y_index)
                {
                    for (size_t x_index = 0; x_index < dpl_x; ++x_index, ++index)
                    {
                        //__file->write((char*)&data[index], sizeof(data[index]));

                        const auto fy = 1.f - std::fabs(int(y_index) - int(dpl_y / 2)) / (float)(dpl_y / 2);
                        const auto fx = 1.f - std::fabs(int(x_index) - int(dpl_x / 2)) / (float)(dpl_x / 2);

                        const float value = 3000.0f * fy * fx * std::fabs(ndist(gen));

                        __file->write((char*)&value, sizeof(value));
                    }
                }
            }
        }

        __file->flush();
        __file->close();
    }
};

class explorer
{
    constexpr static auto readmode = std::ios_base::binary | std::ios_base::in;

    struct fstream_deleter
    {
        void operator()(std::fstream* s)
        {
            assert(s);
            s->close();
            delete s;
        }
    };

    std::unique_ptr<pixel[]> __texture_draw;
    GLuint                   __texture_id;

    std::unique_ptr<uint8_t[]> __texture_save;

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
        return (uint8_t*)__texture_save.get();
    }

    size_t get_texture_size()
    {
        return 54 + 3 * view_width * view_height;
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

        size_t index = 0;

        size_t l_index = 0;
        size_t z_index = 0;
        size_t y_index = 0;
        size_t x_index = 0;

        frame_width = m.dpl_x;
        frame_height = m.dpl_y;

        const auto& matrix = m.__matrix[l_index];

        if (selected_l_index)
        {
            l_index = selected_l_index % m.dpl_l;
            index += m.dpp_l * l_index;
        }

        if (selected_z_index)
        {
            z_index = selected_z_index % m.dpl_z;
            index += m.dpp_z * z_index;
        }

        //for (;l_index < m.num_layers; ++l_index)
        {
            // for (; z_index < m.dpl_z; ++z_index)
            {
                for (y_index = 0; y_index < m.dpl_y; ++y_index)
                {
                    for (x_index = 0; x_index < m.dpl_x; ++x_index, ++index)
                    {
                        const int x = static_cast<int>(x_index);
                        const int y = static_cast<int>(y_index);
                        
                        set_color(matrix[index]);

                        glVertex2i(x, y);
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
                __texture_save.reset(new uint8_t[54 + 3 * view_width * view_height]{ 0 });
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
                std::fstream file("new.bmp", std::ios_base::binary | std::ios_base::out);
                file.write((const char*)__texture_save.get(), 54 + 3 * view_width * view_height);
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