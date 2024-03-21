#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../shader_pipeline/shader_pipeline.hpp" // TODO: don't depend on path
#include <string>
#include <map>

/// Holds all state information relevant to a character as loaded using FreeType
struct CharacterDrawingData {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2 Size;      // Size of glyph
    glm::ivec2 Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

/**
 * configure_opengl_for_text_rendering must be called before any other function is called!
 * \todo make this not the case, do it automatically.
 */
class TextRenderer {
public:
    std::tuple<bool, std::map<GLchar, CharacterDrawingData>> prepare_freetype_font(const std::string& font_path);
    void configure_opengl_for_text_rendering(const unsigned int screen_width, const unsigned int screen_height);
    void render_text(std::map<GLchar, CharacterDrawingData> char_to_drawing_data, std::string text, float x, float y, float scale, glm::vec3 color);

    ShaderPipeline shader_pipeline;
    GLuint vao_name;
    GLuint vbo_name;

};


#endif //TEXT_RENDERER_HPP
