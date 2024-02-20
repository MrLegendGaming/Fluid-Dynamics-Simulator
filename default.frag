#version 330 core
out vec4 FragColor;
uniform vec3 color;
uniform vec2 velocity; // New uniform for velocity

void main()
{
    // Calculate the magnitude of the velocity
    float speed = length(velocity) + 0.35f;

    // Use the magnitude of velocity to influence the color
    vec3 modifiedColor = color * speed; // You can adjust this calculation based on your desired effect

    // Output the modified color
    FragColor = vec4(modifiedColor, 1.0);
}
