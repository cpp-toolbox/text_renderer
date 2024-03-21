#include "text_renderer.hpp"

#include <map>
#include <iostream>

#include <glad/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 *
 * description:
 *  returns a tuple where the first element is whether or not we were
 *  able to successfully prepare the freetype font.
 *
 *  If we were able to successfully prepare the font, then the second
 *  element is a mapping of characters to their drawing data.
 *
 *  If we were not able to successfully prepare the font, then the
 *  second element is an empty map.
 *
 *  \todo don't return success tuple instead throw in this function so errors can be dealt with properly
 */

// todo: rename to generate font information, this "loads" in a font, and then whenever we want to render
// text we just have to pass one of these things in.
// this keeps the font-renderer de-coupled from the font data generation component
std::tuple<bool, std::map<GLchar, CharacterDrawingData>> TextRenderer::prepare_freetype_font(const std::string& font_path) {

    std::map<GLchar, CharacterDrawingData> char_to_drawing_data;

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return {false, char_to_drawing_data};
    }

    // find path to font
    if (font_path.empty()) {
        std::cout << "ERROR::FREETYPE: Failed to load font_path" << std::endl;
        return {false, char_to_drawing_data};
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_path.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return {false, char_to_drawing_data};
    } else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            CharacterDrawingData character = {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<unsigned int>(face->glyph->advance.x)
            };
            char_to_drawing_data.insert(std::pair<char, CharacterDrawingData>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return {true, char_to_drawing_data};
}

void TextRenderer::render_text(std::map<GLchar, CharacterDrawingData> char_to_drawing_data, std::string text, float x, float y, float scale, glm::vec3 color) {
    // activate corresponding render state
    // shader.use();
    glUniform3f(glGetUniformLocation(this->shader_pipeline.shader_program_id, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_name);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {

        CharacterDrawingData ch = char_to_drawing_data[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
                // x position, y position, x texture coordinate, y texture coordinate
                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos,     ypos,     0.0f, 1.0f},
                {xpos + w, ypos,     1.0f, 1.0f},

                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos + w, ypos,     1.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 0.0f}
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo_name);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                        vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) *
             scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// todo: here instead we should just store the opengl data in a class most likely, and what is that class
// it is a font-renderer, which gets passed in
/**
 *
 * @param screen_width
 * @param screen_height
 * @return
 */
void TextRenderer::configure_opengl_for_text_rendering(const unsigned int screen_width, const unsigned int screen_height) {

    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // compile and setup the shader_pipeline
    // ----------------------------
    ShaderPipeline shader_pipeline;
    shader_pipeline.load_in_shaders_from_file("../shaders/text.vert", "../shaders/text.frag");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screen_width), 0.0f, static_cast<float>(screen_height));
    glUniformMatrix4fv(glGetUniformLocation(shader_pipeline.shader_program_id, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    // configure VAO/VBO for texture quads
    // -----------------------------------
    GLuint vao_name, vbo_name;
    glGenVertexArrays(1, &vao_name);
    glGenBuffers(1, &vbo_name);
    glBindVertexArray(vao_name);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_name);
    // 6 because every quad has 6 vertices, and aparently we need 4 of them?
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    // a generic vertex type that contains all information
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->shader_pipeline = shader_pipeline;
    this->vao_name = vao_name;
    this->vbo_name = vbo_name;

}
