#ifndef CIRCLE_H
#define CIRCLE_H

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h> // Include glad to get all the required OpenGL headers

class Circle {
    GLuint shaderProgram;

public:
    float circleRadius;
    Circle(float radius, int numSegments, glm::vec3 color, GLuint shader)
        : radius(radius), numSegments(numSegments), color(color), shaderProgram(shader) {
        generateVertices();
        circleRadius = radius;
    }

    ~Circle() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void draw(glm::vec2 velocity) {
        glBindVertexArray(VAO);
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &color[0]);

        // Pass velocity as a uniform to the shader
        glUniform2fv(glGetUniformLocation(shaderProgram, "velocity"), 1, &velocity[0]);

        glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size());
        glBindVertexArray(0);
    }

private:
    float radius;
    int numSegments;
    glm::vec3 color;
    std::vector<glm::vec2> vertices;
    GLuint VAO, VBO;

    void generateVertices() {
        vertices.push_back(glm::vec2(0.0f, 0.0f));
        for (int i = 0; i <= numSegments; ++i) {
            float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
            float dx = radius * cosf(theta);
            float dy = radius * sinf(theta);
            vertices.push_back(glm::vec2(dx, dy));
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

#endif