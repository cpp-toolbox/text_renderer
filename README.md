# Info

utilities for font rederering as well as text rendering

# Depdendencies
* [freetype](https://github.com/freetype/freetype)
* [glm](https://github.com/g-truc/glm)
* [shader pipeline](https://github.com/opengl-toolbox/shader_pipeline)

# CMake

```
...


# GLM: opengl mathematics

include_directories(external_libraries/glm)
add_subdirectory(external_libraries/glm)

# FreeType: font rendering

add_subdirectory(external_libraries/freetype)
include_directories(external_libraries/freetype/include)


... 

target_link_libraries(your_project_name ... freetype)
```
