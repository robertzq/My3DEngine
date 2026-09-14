# ============ 引擎库 (libengine) ============
# 可复用的 2D 游戏引擎，不依赖任何具体游戏资源/逻辑。

# --- 平台依赖：SDL2 / SDL2_image / SDL2_ttf / SDL2_mixer ---
if(WIN32)
    message(STATUS "Detecting Windows system...")
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_image")
    set(SDL2_TTF_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_ttf")
    set(SDL2_MIXER_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL2_mixer")
    find_library(SDL2_LIB NAMES SDL2 PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_MAIN_LIB NAMES SDL2main PATHS ${SDL2_DIR}/lib/x64)
    find_library(SDL2_IMAGE_LIB NAMES SDL2_image PATHS ${SDL2_IMAGE_DIR}/lib/x64)
    find_library(SDL2_TTF_LIB NAMES SDL2_ttf PATHS ${SDL2_TTF_DIR}/lib/x64)
    find_library(SDL2_MIXER_LIB NAMES SDL2_mixer PATHS ${SDL2_MIXER_DIR}/lib/x64)
    set(ENGINE_INCLUDE_DIRS ${SDL2_DIR}/include ${SDL2_IMAGE_DIR}/include ${SDL2_TTF_DIR}/include ${SDL2_MIXER_DIR}/include)
    set(ENGINE_LIBS ${SDL2_MAIN_LIB} ${SDL2_LIB} ${SDL2_IMAGE_LIB} ${SDL2_TTF_LIB} ${SDL2_MIXER_LIB})
elseif(APPLE)
    message(STATUS "Detecting macOS system...")
    if(EXISTS "/opt/homebrew/include")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
    set(ENGINE_INCLUDE_DIRS ${HOMEBREW_PREFIX}/include ${HOMEBREW_PREFIX}/include/SDL2)
    set(ENGINE_LIBS SDL2 SDL2_image SDL2_ttf SDL2_mixer)
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    set(ENGINE_LIBS SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)
endif()

# --- 引擎静态库 ---
add_library(engine STATIC
    src/Game.cpp
    src/ResourceManager.cpp
    src/TextRenderer.cpp
    src/Physics.cpp
    src/TileMap.cpp
    src/SceneManager.cpp
    src/Entity.cpp
    src/Time.cpp
    src/Input.cpp
    src/Animator.cpp
    src/Audio.cpp
    src/GL.cpp
    src/Shader.cpp
    src/ShaderManager.cpp
    src/Renderer.cpp
    src/RenderTarget.cpp
    src/PostProcess.cpp
    src/PageCurl.cpp
    src/MeshPageCurl.cpp
    src/RedBorder.cpp
    src/UI/UIElement.cpp
    src/UI/UIPrimitives.cpp
    src/UI/UILayout.cpp
    src/UI/UIContainer.cpp
    src/UI/Menu.cpp
    src/UI/MenuInput.cpp
    src/UI/MenuStack.cpp
    src/UI/UIManager.cpp
)

target_include_directories(engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${ENGINE_INCLUDE_DIRS}
)

if(APPLE)
    target_link_directories(engine PUBLIC ${HOMEBREW_PREFIX}/lib)
endif()

target_link_libraries(engine PUBLIC ${ENGINE_LIBS})
