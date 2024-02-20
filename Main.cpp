#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Circle.h"

#include <random>



float random_float() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    return dis(gen);
}

// GLFW Functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// Maths & Physics Functions
int num;
bool useGravity = false;

float mapToRange(float value, float minInput, float maxInput);

void gravity(glm::vec2& velocity);
void updatePosition(glm::vec2& position, const glm::vec2& velocity, float deltaTime);
void checkBorderCollision(glm::vec2& position, glm::vec2& velocity, float radius, float collisionDamping);
void detectCollisions(glm::vec2 positions[], glm::vec2 velocities[], int numParticles, float radius, float deltaTime);
void parallelPhysicsUpdate(glm::vec2 positions[], glm::vec2 velocities[], int numParticles, float radius, float deltaTime);
void updatePositionsRange(glm::vec2 positions[], glm::vec2 velocities[], int start, int end, float deltaTime);
void processCollisionsRange(glm::vec2 positions[], glm::vec2 velocities[], int start, int end, float radius, float deltaTime);

// --------------------- SETTINGS ---------------------
int SCR_WIDTH = 1200;
int SCR_HEIGHT = 675;

float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT;

// --------------------- TIMING MANAGEMENT ---------------------
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// ==============================================================================================================
// =============================================== MAIN FUNCTION ================================================
// ==============================================================================================================

int main()
{
    // --------------------- INITIALIZING GLFW ---------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --------------------- WINDOW CREATION ---------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Defining a monitor
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    // Centerring the window
    glfwSetWindowPos(window, (mode->width - SCR_WIDTH / 2) - (mode->width / 2), (mode->height - SCR_HEIGHT / 2) - (mode->height / 2));

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    double previousTime = glfwGetTime();
    int frameCount = 0;

    // --------------------- SHADER STUFF ---------------------
    // build and compile our shader program
    Shader ourShader("default.vert", "default.frag");

    // --------------------- CIRCLE SETTINGS ---------------------
    Circle circle(0.02f, 100, glm::vec3(0.0f, 1.0f, 1.0f), ourShader.ID);

    glm::vec2 positions[2000];
    constexpr int particleNum = sizeof(positions) / sizeof(positions[0]);

    glm::vec2 velocities[particleNum];

    num = particleNum;
   
    for (int i = 0; i < particleNum; i++)
    {
        positions[i] = glm::vec2(random_float(), random_float());
        if (positions[i].x > 1.0f || positions[i].x < -1.0f || positions[i].y > 1.0f || positions[i].y < -1.0f)
        {
            std::cout << positions[i].x << ", " << positions[i].y << std::endl;
        }
        velocities[i] = glm::vec2(random_float() * 5, random_float() * 2);
        velocities[i] = glm::vec2(0);
    }


    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    float time;
    
    const float twiceRadius = 2.0f * circle.circleRadius;
    const float radiusSquared = circle.circleRadius * circle.circleRadius;
    const float negCollisionDamping = -0.75;
    const float borderMinX = -1.0f + circle.circleRadius;
    const float borderMaxX = 1.0f - circle.circleRadius;
    const float borderMinY = -1.0f + circle.circleRadius;
    const float borderMaxY = 1.0f - circle.circleRadius;

    // ==============================================================================================================
    // ============================================== MAIN WHILE LOOP ===============================================
    // ==============================================================================================================

    std::sort(positions, positions + particleNum, [](const glm::vec2& a, const glm::vec2& b) {
        return a.x < b.x;
        });

    bool spawn = true;

    while (!glfwWindowShouldClose(window))
    {
        time = glfwGetTime();
        // Per-frame time logic
        float currentFrame = static_cast<float>(time);
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;

        if (currentFrame - previousTime >= 1.0)
        {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            previousTime = currentFrame;
        }


        // input
        processInput(window);

        // render
        //glClearColor(0.09804f, 0.10196f, 0.16078f, 1.0f);
        //glClearColor(0.15686f, 0.17647f, 0.23922f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ourShader.use();

        glUniform1f(glGetUniformLocation(ourShader.ID, "aspectRatio"), aspectRatio);
        glm::mat4 projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);

        GLint projLoc = glGetUniformLocation(ourShader.ID, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Get the location of the 'circleColor' uniform in the shader program
        glfwGetWindowSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        
        std::vector<std::pair<int, int>> potentialCollisions;
        // Updated collision detection loop
        /*detectCollisions(positions, velocities, num, circle.circleRadius, deltaTime);*/
        parallelPhysicsUpdate(positions, velocities, num, circle.circleRadius, deltaTime);
        
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            for (int i = 0; i < num; i++)
            {
                velocities[i].x += random_float();
                velocities[i].y += random_float();
            }
        }

        // Assuming 'i' is declared and initialized before this snippet

        // Get mouse position
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        glm::vec2 mousePos = glm::vec2(mouseX / SCR_WIDTH * 2 - 1, 1 - mouseY / SCR_HEIGHT * 2);

        // Clamp mouse position
        mousePos.x = glm::clamp(mousePos.x, -1.0f, 1.0f);
        mousePos.y = glm::clamp(mousePos.y, -1.0f, 1.0f);

        // Determine action based on mouse button
        int leftMouseButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        int rightMouseButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

        // Move particles
        for (int i = 0; i < num; i++)
        {
            glm::vec2 velocityChange = glm::vec2(0.0f); // Default no change in velocity

            float distanceToMouse = glm::length(mousePos - positions[i]);
            if (distanceToMouse <= 0.15f) {
                if (leftMouseButton == GLFW_PRESS) {
                    // Repel particles if left mouse button is pressed
                    velocityChange = glm::normalize(positions[i] - mousePos);
                }
                else if (rightMouseButton == GLFW_PRESS) {
                    // Attract particles if right mouse button is pressed
                    velocityChange = glm::normalize(mousePos - positions[i]);
                }
            }

            // Apply velocity change to particles
            velocities[i] += velocityChange * 0.25f;
            // You may want to clamp the velocity to a maximum value if needed
            // velocities[i] = glm::clamp(velocities[i], -maxSpeed, maxSpeed);

            // Update particle positions based on velocity
            positions[i] += velocities[i] * deltaTime; // deltaTime is the time elapsed since the last frame

            // Render particles using OpenGL commands
        }


        for (int i = 0; i < particleNum; i++)
        {
            glm::mat4 transform = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            
            if (useGravity) {
                gravity(velocities[i]);
            }

            checkBorderCollision(positions[i], velocities[i], circle.circleRadius, 0.75f);
            updatePosition(positions[i], velocities[i], deltaTime);

            transform = glm::translate(transform, glm::vec3(positions[i], 0.0f));

            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
            circle.draw(velocities[i]);
        }
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    
    return 0;
}


// ===========================================================================================================
// ================================================ FUNCTIONS ================================================
// ===========================================================================================================


// --------------------- GLFW FUNCTIONS ---------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        useGravity = false;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        useGravity = true;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// --------------------- MATHS FUNCTIONS ---------------------
float mapToRange(float value, float minInput, float maxInput) {
    // Calculate the range of the input values
    float inputRange = maxInput - minInput;

    // Calculate the normalized value within the input range
    float normalizedValue = (value - minInput) / inputRange;

    // Map the normalized value to the range [-1.0, 1.0]
    return normalizedValue * 2.0f - 1.0f;
}

// --------------------- PHYSICS FUNCTIONS ---------------------

void gravity(glm::vec2& velocity)
{
    velocity.y += -9.81 * deltaTime * 0.25;

}

void updatePosition(glm::vec2& position, const glm::vec2& velocity, float deltaTime)
{
    position += velocity * deltaTime;
}

void checkBorderCollision(glm::vec2& position, glm::vec2& velocity, float radius, float collisionDamping)
{
    // Precompute constants
    const float negCollisionDamping = -collisionDamping;
    const float borderMinX = -1.0f + radius;
    const float borderMaxX = 1.0f - radius;
    const float borderMinY = -1.0f + radius;
    const float borderMaxY = 1.0f - radius;

    // Handle border collisions
    const glm::vec2 clampedPosition = glm::clamp(position, glm::vec2(borderMinX, borderMinY), glm::vec2(borderMaxX, borderMaxY));

    const bool borderCollisionX = position.x != clampedPosition.x;
    const bool borderCollisionY = position.y != clampedPosition.y;

    position = clampedPosition;

    if (borderCollisionX) {
        velocity.x *= negCollisionDamping;
    }
    if (borderCollisionY) {
        velocity.y *= negCollisionDamping;
    }

}

void detectCollisions(glm::vec2 positions[], glm::vec2 velocities[], int numParticles, float radius, float deltaTime) {
    const float collisionRadius = 2.0f * radius; // Adjust this value as needed
    for (int i = 0; i < numParticles - 1; ++i) {
        for (int j = i + 1; j < numParticles; ++j) {
            // Calculate squared distance between particles
            glm::vec2 direction = positions[j] - positions[i];
            float distanceSquared = glm::dot(direction, direction);

            // If particles are within collision range
            if (distanceSquared < collisionRadius * collisionRadius) {
                // Collision detection and response logic
                const float distance = sqrt(distanceSquared);
                const glm::vec2 normal = direction / distance;
                const glm::vec2 relativeVelocity = velocities[j] - velocities[i];
                const float velocityAlongNormal = glm::dot(relativeVelocity, normal);
                if (velocityAlongNormal < 0) {
                    const float elasticity = 0.9f; // coefficient of restitution
                    const float impulseMagnitude = -(1.0f + elasticity) * velocityAlongNormal / 2.0f; // simplified for equal mass
                    const glm::vec2 impulse = impulseMagnitude * normal;
                    velocities[i] -= impulse; // Apply impulse to the first particle
                    velocities[j] += impulse; // Apply impulse to the second particle

                    // Adjust particle positions to prevent overlap
                    const glm::vec2 correction = (collisionRadius - distance) * 0.5f * normal;
                    positions[i] -= correction;
                    positions[j] += correction;
                }
            }
        }
    }
}

// Function to process collisions for a range of particles
void processCollisionsRange(glm::vec2 positions[], glm::vec2 velocities[], int start, int end, float radius, float deltaTime) {
    const float collisionRadius = 2.0f * radius; // Adjust this value as needed
    for (int i = start; i < end - 1; ++i) {
        for (int j = i + 1; j < end; ++j) {
            // Calculate squared distance between particles
            glm::vec2 direction = positions[j] - positions[i];
            float distanceSquared = glm::dot(direction, direction);

            // If particles are within collision range
            if (distanceSquared < collisionRadius * collisionRadius) {
                // Collision detection and response logic
                const float distance = sqrt(distanceSquared);
                const glm::vec2 normal = direction / distance;
                const glm::vec2 relativeVelocity = velocities[j] - velocities[i];
                const float velocityAlongNormal = glm::dot(relativeVelocity, normal);
                if (velocityAlongNormal < 0) {
                    const float elasticity = 0.9f; // coefficient of restitution
                    const float impulseMagnitude = -(1.0f + elasticity) * velocityAlongNormal / 2.0f; // simplified for equal mass
                    const glm::vec2 impulse = impulseMagnitude * normal;
                    velocities[i] -= impulse; // Apply impulse to the first particle
                    velocities[j] += impulse; // Apply impulse to the second particle

                    // Adjust particle positions to prevent overlap
                    const glm::vec2 correction = (collisionRadius - distance) * 0.5f * normal;
                    positions[i] -= correction;
                    positions[j] += correction;
                }
            }
        }
    }
}

// Function to update particle positions for a range of particles
void updatePositionsRange(glm::vec2 positions[], glm::vec2 velocities[], int start, int end, float deltaTime) {
    for (int i = start; i < end; ++i) {
        // Update particle positions based on velocity
        positions[i] += velocities[i] * deltaTime; // deltaTime is the time elapsed since the last frame
    }
}

// Function to parallelize collision detection and particle movement
void parallelPhysicsUpdate(glm::vec2 positions[], glm::vec2 velocities[], int numParticles, float radius, float deltaTime) {
    const int numThreads = std::thread::hardware_concurrency();
    const int particlesPerThread = numParticles / numThreads;
    std::vector<std::thread> threads;

    // Launch threads to process collisions and update positions for different ranges of particles
    for (int i = 0; i < numThreads; ++i) {
        int start = i * particlesPerThread;
        int end = (i == numThreads - 1) ? numParticles : (i + 1) * particlesPerThread;
        threads.emplace_back(processCollisionsRange, std::ref(positions), std::ref(velocities), start, end, radius, deltaTime);
        threads.emplace_back(updatePositionsRange, std::ref(positions), std::ref(velocities), start, end, deltaTime);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}