#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <map>

#include "sbpt_generated_includes.hpp"

/// Holds all state information relevant to a character as loaded using FreeType
struct CharacterDrawingData {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;        // (WIDTH, HEIGHT)
    glm::ivec2 Bearing;     // (BEARING_X, BEARING_Y)
    unsigned int Advance;   // ADVANCE
};

/**
 * configure_opengl_for_text_rendering must be called before any other function is called!
 * \todo make this not the case, do it automatically.
 */

using GLCharToDrawingData = std::map<GLchar, CharacterDrawingData>;
class TextRenderer {
  public:
    TextRenderer(const std::string &font_path, unsigned int font_height_px, unsigned int &window_width_px,
                 unsigned int &window_height_px, ShaderCache &shader_cache);

    GLCharToDrawingData generate_font_data(const std::string &font_path, unsigned int font_height_px);

    void render_text(const std::string &text, glm::vec2 ndc_coord, float scale, const glm::vec3 &color);

    glm::vec2 get_text_dimensions_in_ndc(const std::string &text, float scale) const;

    unsigned int num_vertices_per_quad = 6;
    unsigned int &window_width_px, &window_height_px;
    GLCharToDrawingData gl_char_to_drawing_data;
    ShaderCache &shader_cache;
    GLuint vertex_attribute_object, vertex_position_buffer_object, texture_coordinate_buffer_object;
};

#endif // TEXT_RENDERER_HPP
