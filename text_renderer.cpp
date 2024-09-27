#include "text_renderer.hpp"

#include <map>
#include <iostream>

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 *......................................................................................................................
 *...GLYPH.METRICS...................................................ASCII ART CREATED BY CUPPAJOEMAN.COM...............
 *........................X_MIN.............................X_MAX.......................................................
 *..............│...........│.................................│.........................................................
 *..............│...........│.................................│.........................................................
 *..............│...........│.................................│.........................................................
 *..............│...........│◄───────────WIDTH───────────────►│.........................................................
 *..............│...........│...............................+.│.........................................................
 *..............│...........│.................................│.........................................................
 *..............│...........┼─────────────────────────────────┼──────────────────────────────────────────────────►.Y_MAX
 *..............│...........│..........@@@@@@@@@@@............│.....▲......................................▲............
 *..............│...........│.......*@@@+......:@@@@@@@@@@@@@*│.....│.-....................................│............
 *..............│...........│.....@@@@@..........@@@@@@@@@@@@@│.....│......................................│............
 *..............│◄BEARING_X►│....@@@@@............%@@@@@......│.....│......................................│............
 *..............│...........│...=@@@@@.............@@@@@@.....│.....│......................................│............
 *..............│...........│...@@@@@@.............@@@@@@.....│.....│......................................│............
 *..............│...........│...@@@@@@.............@@@@@@.....│.....│......................................│............
 *..............│...........│...@@@@@@.............@@@@@@.....│.....│......................................│............
 *..............│...........│...-@@@@@%............@@@@@%.....│..BEARING_Y.................................│............
 *..............│...........│....%@@@@@-..........*@@@@%......│.....│......................................│............
 *..............│...........│.....:@@@@@-........+@@@@-.......│.....│......................................│............
 *..............│...........│.......-@@@@@#=...%@@@@-.........│.....│......................................│............
 *..............│...........│.......=@@...#####+..............│.....│......................................│............
 *..............│...........│.....%@@.........................│.....│...................................HEIGHT..........
 *..............│...........│...:@@@%.........................│.....│......................................│............
 *..............│...........│...@@@@@@@%%%%%%%%%%.............│.....│......................................│............
 *..............│...........│...@@@@@@@@@@@@@@@@@@@@@@@@@@*...│.....│......................................│............
 *..............│...........│....:@@@@@@@@@@@@@@@@@@@@@@@@@@+.│.....▼......................................│............
 *.....─────────@───────────┼──────@@─────────────────@@@@@@@─┼──────────────────@────────────►............│............
 *......ORIGIN..│...........│....#@@......................*@@@│.....▲............│.........................│............
 *..............│...........│...@@%........................@@@│.....│............│.........................│............
 *..............│...........│.=@@@.........................@@.│.....│............│.........................│............
 *..............│...........│.@@@@%......................:@@-.│.....│............│.........................│............
 *..............│...........│@@@@@@@@..................:@@@...│..UNDERHANG.......│.........................│............
 *..............│...........│.#@@@@@@@@@@@@+++++++@@@@@@@.....│.....│............│.........................│............
 *..............│...........│....@@@@@@@@@@@@@@@@@@@@@-.......│.....│............│.........................│............
 *..............│...........│........+@@@@@@@@@@@++...........│.....▼............│.........................▼............
 *..............│...........└─────────────────────────────────┴──────────────────┼───────────────────────────────►.Y_MIN
 *..............│................................................................│......................................
 *..............│................................................................│......................................
 *..............│◄─────────────────────────ADVANCE──────────────────────────────►│......................................
 *..............│................................................................│......................................
 *..............│................................................................│......................................
 *..............│................................................................│......................................
 *......................................................................................................................
 * LEGEND:
 *  - BEARING_X: the horizontal offset to the leftmost pixel of the characters bitmap from the origin
 *  - BEARING_Y: the vertical offset of the topmost pixel relative to the baseline
 *  - HEIGHT: the number of pixels inclusive from the bottommost pixel to the topmost pixel
 *  - WIDTH: the number of pixels inclusive from the leftmost pixel to the rightmost pixel
 *  - ADVANCE: the horizontal distance from the origin to the origin of the next character to be drawn
 *  - UNDERHANG: how much of the glyph goes below the baseline, can be compued via HEIGHT - BEARING_Y
 */

TextRenderer::TextRenderer(const std::string &font_path, unsigned int font_height_px, unsigned int &window_width_px,
                           unsigned int &window_height_px, ShaderCache &shader_cache)
    : shader_cache(shader_cache), window_width_px(window_width_px), window_height_px(window_height_px),
      gl_char_to_drawing_data(generate_font_data(font_path, font_height_px)) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &vertex_attribute_object);
    glGenBuffers(1, &vertex_position_buffer_object);
    glGenBuffers(1, &texture_coordinate_buffer_object);

    glBindVertexArray(vertex_attribute_object);

    // pre allocate the space for the vertex buffers, we don't actually store anything in them,
    // then later on we call glBufferSubData to store the data
    glBindBuffer(GL_ARRAY_BUFFER, vertex_position_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * num_vertices_per_quad, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, texture_coordinate_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * num_vertices_per_quad, NULL, GL_DYNAMIC_DRAW);

    shader_cache.configure_vertex_attributes_for_drawables_vao(vertex_attribute_object, vertex_position_buffer_object,
                                                               ShaderType::TEXT,
                                                               ShaderVertexAttributeVariable::XY_POSITION);

    shader_cache.configure_vertex_attributes_for_drawables_vao(
        vertex_attribute_object, texture_coordinate_buffer_object, ShaderType::TEXT,
        ShaderVertexAttributeVariable::PASSTHROUGH_TEXTURE_COORDINATE);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

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

GLCharToDrawingData TextRenderer::generate_font_data(const std::string &font_path, unsigned int font_height_px) {

    std::map<GLchar, CharacterDrawingData> char_to_drawing_data;

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        spdlog::error("Could not initialize FreeType library");
        throw std::runtime_error("Could not initialize FreeType library");
    }

    if (font_path.empty()) {
        spdlog::error("Failed to load font. Provided font path is empty.");
        FT_Done_FreeType(ft);
        throw std::invalid_argument("Font path is empty");
    }

    FT_Face face;
    if (FT_New_Face(ft, font_path.c_str(), 0, &face)) {
        spdlog::error("Failed to load font from path: {}", font_path);
        FT_Done_FreeType(ft);
        throw std::runtime_error("Failed to load font");
    }

    FT_Set_Pixel_Sizes(face, 0, font_height_px);

    // the bitmap generated from the glyph is a grayscale 8-bit image where each color is represented by a single byte.
    // For this reason we'd like to store each byte of the bitmap buffer as the texture's single color value. We
    // accomplish this by creating a texture where each byte corresponds to the texture color's red component (first
    // byte of its color vector). If we use a single byte to represent the colors of a texture we do need to take care
    // of a restriction of OpenGL:
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            spdlog::warn("Failed to load glyph for character: '{}'", c);
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED,
                     GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        CharacterDrawingData character = {texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                                          glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                                          static_cast<unsigned int>(face->glyph->advance.x)};
        char_to_drawing_data[c] = character;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return char_to_drawing_data;
}

void TextRenderer::render_text(const std::string &text, float x, float y, float scale, const glm::vec3 &color) {
    // allow text to appear on top of everything
    glDisable(GL_DEPTH_TEST);
    // Activate corresponding render state
    shader_cache.use_shader_program(ShaderType::TEXT);
    shader_cache.set_uniform(ShaderType::TEXT, ShaderUniformVariable::RGB_COLOR, color);
    glm::mat4 orthographic_camera_screen_coordinates =
        glm::ortho(0.0f, static_cast<float>(window_width_px), 0.0f, static_cast<float>(window_height_px));
    shader_cache.set_uniform(ShaderType::TEXT, ShaderUniformVariable::CAMERA_TO_CLIP,
                             orthographic_camera_screen_coordinates);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vertex_attribute_object);

    // Vectors for vertices and texture coordinates
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> texture_coordinates;

    // Reserve space to avoid frequent reallocations
    vertices.reserve(num_vertices_per_quad);
    texture_coordinates.reserve(num_vertices_per_quad);

    // Iterate through all characters
    for (const char &c : text) {
        const CharacterDrawingData &ch = gl_char_to_drawing_data[c];

        const float xpos = x + ch.Bearing.x * scale;

        // see the huge comment for the definition of what underhang is
        const float scaled_underhang = (ch.Size.y - ch.Bearing.y) * scale;
        const float ypos = y - scaled_underhang;

        const float w = ch.Size.x * scale;
        const float h = ch.Size.y * scale;

        // Clear vectors to reuse allocated memory
        vertices.clear();
        texture_coordinates.clear();

        // Add vertices for each character
        vertices.push_back(glm::vec2(xpos, ypos + h));
        vertices.push_back(glm::vec2(xpos, ypos));
        vertices.push_back(glm::vec2(xpos + w, ypos));

        vertices.push_back(glm::vec2(xpos, ypos + h));
        vertices.push_back(glm::vec2(xpos + w, ypos));
        vertices.push_back(glm::vec2(xpos + w, ypos + h));

        // Add texture coordinates for each character
        texture_coordinates.push_back(glm::vec2(0.0f, 0.0f));
        texture_coordinates.push_back(glm::vec2(0.0f, 1.0f));
        texture_coordinates.push_back(glm::vec2(1.0f, 1.0f));

        texture_coordinates.push_back(glm::vec2(0.0f, 0.0f));
        texture_coordinates.push_back(glm::vec2(1.0f, 1.0f));
        texture_coordinates.push_back(glm::vec2(1.0f, 0.0f));

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory for vertices
        glBindBuffer(GL_ARRAY_BUFFER, vertex_position_buffer_object);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec2), vertices.data());

        // Update content of VBO memory for texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, texture_coordinate_buffer_object);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coordinates.size() * sizeof(glm::vec2), texture_coordinates.data());

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, num_vertices_per_quad);

        // Advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64, divide amount of 1/64th
                                        // pixels by 64 to get amount of pixels)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shader_cache.stop_using_shader_program();
    glEnable(GL_DEPTH_TEST);
}
