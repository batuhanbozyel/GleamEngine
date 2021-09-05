add_subdirectory(imgui)
add_subdirectory(SDL2)

set_target_properties (
    SDL2
    SDL2main
    SDL2-static
    imgui
    PROPERTIES FOLDER Dependencies)