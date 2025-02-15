#version 410

in vec3 particleColor;
out vec4 fragColor;

void main() {
    // Create a circular particle
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float circle = dot(circCoord, circCoord);
    if (circle > 1.0) {
        discard;
    }
    
    fragColor = vec4(particleColor, 0.3);  // More transparent
}