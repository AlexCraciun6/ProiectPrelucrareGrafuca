#version 410

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 velocity;
layout(location = 2) in float randomOffset;  // Add randomness to particles

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float deltaTime;
uniform float currentTime;

out vec3 particleColor;

void main() {
    // Use random offset for varied falling pattern
    float timeOffset = currentTime + randomOffset;
    vec3 newPosition = position;
    newPosition.y = position.y - mod(timeOffset * 30.0, 110.0);  // 30.0 is fall speed
    
    gl_Position = projection * view * model * vec4(newPosition, 1.0);
    
    // Fade color based on y position
    float alpha = (newPosition.y + 10.0) / 110.0;
    particleColor = vec3(0.6, 0.6, 1.0) * alpha;
    
    gl_PointSize = 2.5;  // Smaller raindrops
}