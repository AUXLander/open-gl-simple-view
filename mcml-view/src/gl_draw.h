#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <fstream>
#include <cassert>
#include <vector>
#include "mcml_types.h"


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

public:

    //std::vector<point_t> __storage;

    std::vector<std::vector<point_t>> __global_storage;

    GLuint textureID;

    int width;
    int height;

    bool draw_include_points {true};

    int index = 0;

    size_t min_layer = 0;
    size_t max_layer = 0xffff;

    std::unique_ptr<pixel> texture;

    void init(int ViewWidth, int ViewHeight)
    {
        width = ViewWidth;
        height = ViewHeight;

        glShadeModel(GL_SMOOTH);
        glMatrixMode(GL_PROJECTION_MATRIX);
        glLoadIdentity();
        glOrtho(0, height, 0, width, -1, 1);
        glViewport(0, 0, height, width);
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

        // headers
        short num_layers = 0;
        long  num_photons = 0;

        double max_x = 0;
        double max_y = 0;
        double max_z = 0;

        double min_x = 0;
        double min_y = 0;
        double min_z = 0;

        __file->read((char*)&num_layers, sizeof(num_layers));
        __file->read((char*)&num_photons, sizeof(num_photons));

        __file->read((char*)&max_x, sizeof(max_x));
        __file->read((char*)&max_y, sizeof(max_y));
        __file->read((char*)&max_z, sizeof(max_z));

        __file->read((char*)&min_x, sizeof(min_x));
        __file->read((char*)&min_y, sizeof(min_y));
        __file->read((char*)&min_z, sizeof(min_z));

        //int t = __file->tellg();

        for (long photon_idx = 0; photon_idx < num_photons; ++photon_idx)
        {
            std::vector<point_t> storage;

            size_t id = 0;
            size_t sz = 0;

            __file->read((char*)&id, sizeof(id));
            __file->read((char*)&sz, sizeof(sz));

            storage.reserve(sz);

            while (sz--)
            {
                auto& p = storage.emplace_back();
             
                *__file >> p;

                p.x = (p.x - min_x) / (max_x - min_x);
                p.y = (p.y - min_y) / (max_y - min_y);
                p.z = (p.z - min_z) / (max_z - min_z);
            }

            __global_storage.emplace_back(std::move(storage));
        }
    }

    void Load2DTexture()
    {
        const GLenum target = GL_TEXTURE_2D;
        const GLenum format = GL_RGBA;

        if (texture)
        {
            glEnable(target);
            glGenTextures(1, &textureID);
            glBindTexture(target, textureID);

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            glTexImage2D(target, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, texture.get());

            glDisable(target);
        }
    }

    void render_frame()
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (const auto& storage : __global_storage)
        {
            glEnable(GL_LINE_SMOOTH);

            glColor4f(0.0, 0.0, 0.0, 0.5);
            glLineWidth(1);

            const point_t* first = nullptr;
            const point_t* last = nullptr;

            glBegin(GL_LINE_STRIP);
            for (const auto& v : storage)
            {
                if (min_layer <= v.layer && v.layer <= max_layer)
                {
                    if (!first)
                    {
                        first = &v;
                    }

                    glVertex2f(v.x * width, v.y * height);

                    last = &v;
                }
            }
            glEnd();

            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_BLEND);

            if (draw_include_points && last)
            {
                glPointSize(2);

                glEnable(GL_POINT_SMOOTH);
            
                glBegin(GL_POINTS);

                glColor4f(1.0, 0.0, 0.0, 1.0);
                glVertex2f(first->x * width, first->y * width);

                glColor4f(0.0, 0.0, 1.0, 1.0);
                glVertex2f(last->x * width, last->y * width);

                glEnd();

                glDisable(GL_POINT_SMOOTH);
            }
        }

        // save texture
        if (!texture)
        {
            texture.reset(new pixel[width * height]);
        }

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture.get());
    }

    void DrawTexture()
    {
        if (!index)
        {
            render_frame();
            Load2DTexture();

            index = 1;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glBegin(GL_QUADS);

        glColor3f(1, 1, 1);

        glTexCoord2f(0, 0);
        glVertex2i(0, 0);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2i(0, height);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2i(width, height);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2i(width, 0);

        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
};