cmake_minimum_required(VERSION 3.15)
project(cg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
        dependencies/freeglut/include
        dependencies/glm/include
        engine
)

add_definitions(-DGL_SILENCE_DEPRECATION -DGLM_ENABLE_EXPERIMENTAL)

if(APPLE)
    find_package(OpenGL REQUIRED)
    link_directories(/opt/homebrew/lib)

    include_directories(/opt/homebrew/include)
    add_definitions(-DGLFW_INCLUDE_GLCOREARB)
    add_definitions(-DGLFW_INCLUDE_GLEXT)
endif()

add_executable(cg
        client/main.cpp
        engine/Camera.cpp
        engine/DirectionalLight.cpp
        engine/Light.cpp
        engine/Material.cpp
        engine/Mesh.cpp
        engine/Node.cpp
        engine/Object.cpp
        engine/OrthographicCamera.cpp
        engine/PerspectiveCamera.cpp
        engine/PointLight.cpp
        engine/SpotLight.cpp
        engine/Texture.cpp
        engine/engine.cpp
        engine/List.cpp
        engine/ListElement.cpp
        engine/OvoReader.cpp
        engine/Vertex.cpp
        engine/Vertex.h
        engine/CallbackManager.cpp
)

if(APPLE)
    target_link_libraries(cg
            ${OPENGL_LIBRARIES}
            glut
            freeimage
    )
endif()

set(DEMO_DIR ${CMAKE_SOURCE_DIR}/demo)

add_custom_target(create_demo_dir ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEMO_DIR}
)

add_dependencies(cg create_demo_dir)

add_custom_command(
        TARGET cg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying executable to demo directory..."
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cg> ${DEMO_DIR}/
        VERBATIM
)

if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    add_custom_command(
            TARGET cg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying resources to demo directory..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${DEMO_DIR}/resources
            VERBATIM
    )
endif()