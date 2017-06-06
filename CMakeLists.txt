cmake_minimum_required(VERSION 3.0)
project(Nuklear)

if(UNIX)
    include_directories("${CMAKE_SOURCE_DIR}")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLEW REQUIRED glew)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(OpenGL REQUIRED gl)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(ALLEGRO REQUIRED allegro-5
                                       allegro_image-5
                                       allegro_primitives-5
                                       allegro_main-5
                                       allegro_ttf-5
                                       allegro_font-5)

    file(GLOB ALLEGRO5_SOURCE ${CMAKE_SOURCE_DIR}/demo/allegro5/*.c)
    add_executable(allegro5 ${ALLEGRO5_SOURCE})
    set_target_properties(allegro5 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/allegro5/bin/)

    file(GLOB GLFW_OPENGL2_SOURCE ${CMAKE_SOURCE_DIR}/demo/glfw_opengl2/*.c)
    add_executable(glfw_opengl2 ${GLFW_OPENGL2_SOURCE})
    set_target_properties(glfw_opengl2 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/glfw_opengl2/bin/)

    file(GLOB GLFW_OPENGL3_SOURCE ${CMAKE_SOURCE_DIR}/demo/glfw_opengl3/*.c)
    add_executable(glfw_opengl3 ${GLFW_OPENGL3_SOURCE})
    set_target_properties(glfw_opengl3 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/glfw_opengl3/bin/)

    file(GLOB SDL_OPENGL2_SOURCE ${CMAKE_SOURCE_DIR}/demo/sdl_opengl2/*.c)
    add_executable(sdl_opengl2 ${SDL_OPENGL2_SOURCE})
    set_target_properties(sdl_opengl2 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/sdl_opengl2/bin/)

    file(GLOB SDL_OPENGL3_SOURCE ${CMAKE_SOURCE_DIR}/demo/sdl_opengl3/*.c)
    add_executable(sdl_opengl3 ${SDL_OPENGL3_SOURCE})
    set_target_properties(sdl_opengl3 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/sdl_opengl3/bin/)

    file(GLOB X11_SOURCE ${CMAKE_SOURCE_DIR}/demo/x11/*.c)
    add_executable(x11 ${X11_SOURCE})
    set_target_properties(x11 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/x11/bin/)

    file(GLOB X11_OPENGL2_SOURCE ${CMAKE_SOURCE_DIR}/demo/x11_opengl2/*.c)
    add_executable(x11_opengl2 ${X11_OPENGL2_SOURCE})
    set_target_properties(x11_opengl2 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/x11_opengl2/bin/)

    file(GLOB X11_OPENGL3_SOURCE ${CMAKE_SOURCE_DIR}/demo/x11_opengl3/*.c)
    add_executable(x11_opengl3 ${X11_OPENGL3_SOURCE})
    set_target_properties(x11_opengl3 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/demo/x11_opengl3/bin/)

    target_include_directories(allegro5 PUBLIC "${CMAKE_SOURCE_DIR}/demo/allegro5/")
    target_link_libraries(allegro5 ${ALLEGRO_LIBRARIES} m)

    target_include_directories(glfw_opengl2 PUBLIC "${CMAKE_SOURCE_DIR}/demo/glfw_opengl2/")
    target_link_libraries(glfw_opengl2 ${GLEW_LIBRARIES} ${GLFW_LIBRARIES})

    target_include_directories(glfw_opengl3 PUBLIC "${CMAKE_SOURCE_DIR}/demo/glfw_opengl3/")
    target_link_libraries(glfw_opengl3 ${GLEW_LIBRARIES} ${GLFW_LIBRARIES})

    target_include_directories(sdl_opengl2 PUBLIC "${CMAKE_SOURCE_DIR}/demo/sdl_opengl2/")
    target_link_libraries(sdl_opengl2 ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})

    target_include_directories(sdl_opengl3 PUBLIC "${CMAKE_SOURCE_DIR}/demo/sdl_opengl3/")
    target_link_libraries(sdl_opengl3 ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})

    target_include_directories(x11 PUBLIC "${CMAKE_SOURCE_DIR}/demo/x11/")
    target_link_libraries(x11 ${X11_LIBRARIES})

    target_include_directories(x11_opengl2 PUBLIC "${CMAKE_SOURCE_DIR}/demo/x11_opengl2/")
    target_link_libraries(x11_opengl2 ${GLEW_LIBRARIES} ${X11_LIBRARIES})

    target_include_directories(x11_opengl3 PUBLIC "${CMAKE_SOURCE_DIR}/demo/x11_opengl3/")
    target_link_libraries(x11_opengl3 ${GLEW_LIBRARIES} ${X11_LIBRARIES})



endif()