#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>

struct pixel
{
    uint8_t r{ 0 };
    uint8_t g{ 0 };
    uint8_t b{ 0 };

    uint8_t a{ 255 };

    pixel() = default;

    pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
        r{r}, g{g}, b{b}, a{ a }
    {;}
};

class explorer
{
public:

    GLuint textureID;

    int width = 640;
    int height = 480;

    int index = 0;

    std::unique_ptr<pixel> texture;

    void init(int ViewWidth = 640, int ViewHeight = 480)
    {
        glShadeModel(GL_SMOOTH);
        glMatrixMode(GL_PROJECTION_MATRIX);
        glLoadIdentity();
        glOrtho(0, height, 0, width, -1, 1);
        glViewport(0, 0, ViewWidth, ViewHeight);
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
        if (!texture)
        {
            auto length = width * height;

            texture.reset(new pixel[length]);
        }

        auto* data = texture.get();

        for (int i = 0; i < height; ++i)
        {
            int offset = i * width;

            for (int j = 0; j < width; ++j)
            {
                new (&data[offset + j]) pixel(i, j, 0);
            }
        }
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