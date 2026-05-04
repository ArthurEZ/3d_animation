#include "graphics.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

GLuint create_shader_program(const char* vertex_src, const char* fragment_src) {
    // Compile vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_src, nullptr);
    glCompileShader(vertex);

    int success;
    char info_log[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, nullptr, info_log);
        std::cout << "Vertex shader compilation failed: " << info_log << std::endl;
    }

    // Compile fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_src, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, nullptr, info_log);
        std::cout << "Fragment shader compilation failed: " << info_log << std::endl;
    }

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        std::cout << "Shader program linking failed: " << info_log << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

void setup_camera(const Vec3& camera_pos, const Vec3& target, float aspect_ratio) {
    glm::vec3 pos(camera_pos.x, camera_pos.y, camera_pos.z);
    glm::vec3 tgt(target.x, target.y, target.z);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(pos, tgt, up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    // These would be passed to the shader
    // For now, we'll handle this in the render function
}

void render_character(const AnimatedCharacter& character, const glm::mat4& world_matrix, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, GLuint shader_program) {
    glUseProgram(shader_program);

    GLint world_loc = glGetUniformLocation(shader_program, "world");
    glUniformMatrix4fv(world_loc, 1, GL_FALSE, glm::value_ptr(world_matrix));

    GLint view_loc = glGetUniformLocation(shader_program, "view");
    if (view_loc >= 0) {
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
    }

    GLint projection_loc = glGetUniformLocation(shader_program, "projection");
    if (projection_loc >= 0) {
        glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
    }

    if (!character.bone_transforms.empty()) {
        const GLint bone_loc = glGetUniformLocation(shader_program, "bone_transforms");
        if (bone_loc >= 0) {
            const int count = std::min(static_cast<int>(character.bone_transforms.size()), 100);
            glUniformMatrix4fv(bone_loc, count, GL_FALSE, reinterpret_cast<const float*>(character.bone_transforms.data()));
        }
    }

    for (const auto& primitive : character.mesh.primitives) {
        if (primitive.vao == 0) {
            continue;
        }

        glActiveTexture(GL_TEXTURE0);
        if (primitive.texture_id >= 0 && primitive.texture_id < static_cast<int>(character.mesh.textures.size())) {
            glBindTexture(GL_TEXTURE_2D, character.mesh.textures[primitive.texture_id].gl_id);
            glUniform1i(glGetUniformLocation(shader_program, "texture1"), 0);
            glUniform1i(glGetUniformLocation(shader_program, "useTexture"), 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            glUniform1i(glGetUniformLocation(shader_program, "useTexture"), 0);
        }

        glBindVertexArray(primitive.vao);
        glDrawElements(GL_TRIANGLES, primitive.indices.size(), GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void clear_screen(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GridMesh create_grid(float size, int grid_count) {
    GridMesh grid;
    
    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;
    
    float half_size = size / 2.0f;
    float step = size / grid_count;
    
    // Generate grid vertices
    for (int i = 0; i <= grid_count; ++i) {
        float offset = -half_size + i * step;
        
        // Horizontal lines (along X)
        vertices.push_back(glm::vec3(-half_size, 0.0f, offset));
        vertices.push_back(glm::vec3(half_size, 0.0f, offset));
        
        // Vertical lines (along Z)
        vertices.push_back(glm::vec3(offset, 0.0f, -half_size));
        vertices.push_back(glm::vec3(offset, 0.0f, half_size));
    }
    
    // Generate indices for line segments
    int line_count = (grid_count + 1) * 2;
    for (int i = 0; i < line_count; ++i) {
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);
    }
    
    grid.vertex_count = vertices.size();
    grid.index_count = indices.size();
    
    // Setup VAO and VBO
    glGenVertexArrays(1, &grid.vao);
    glGenBuffers(1, &grid.vbo);
    glGenBuffers(1, &grid.ebo);
    
    glBindVertexArray(grid.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return grid;
}

void render_grid(const GridMesh& grid, const glm::mat4& view_matrix, const glm::mat4& projection_matrix, GLuint shader_program) {
    glUseProgram(shader_program);
    
    glm::mat4 world = glm::identity<glm::mat4>();
    
    GLint world_loc = glGetUniformLocation(shader_program, "world");
    glUniformMatrix4fv(world_loc, 1, GL_FALSE, glm::value_ptr(world));
    
    GLint view_loc = glGetUniformLocation(shader_program, "view");
    if (view_loc >= 0) {
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));
    }
    
    GLint projection_loc = glGetUniformLocation(shader_program, "projection");
    if (projection_loc >= 0) {
        glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));
    }
    
    // Disable texture for grid
    glUniform1i(glGetUniformLocation(shader_program, "useTexture"), 0);
    
    glBindVertexArray(grid.vao);
    glDrawElements(GL_LINES, grid.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void destroy_grid(GridMesh& grid) {
    if (grid.vao != 0) {
        glDeleteBuffers(1, &grid.vbo);
        glDeleteBuffers(1, &grid.ebo);
        glDeleteVertexArrays(1, &grid.vao);
        grid.vao = 0;
        grid.vbo = 0;
        grid.ebo = 0;
    }
}
