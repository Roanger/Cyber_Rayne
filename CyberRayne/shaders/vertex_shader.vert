#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

// Push constants for per-sprite transform, color, and UV region
layout(push_constant) uniform PushConstants {
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
    float u0;
    float v0;
    float uScale;
    float vScale;
} sprite;

layout(location = 1) out vec4 fragColor;

void main() {
    // Scale the unit quad (-0.5 to 0.5) by sprite dimensions and translate
    vec2 scaledPos = inPosition * vec2(sprite.width, sprite.height);
    vec2 finalPos = scaledPos + vec2(sprite.x, sprite.y);

    gl_Position = vec4(finalPos, 0.0, 1.0);
    fragTexCoord = inTexCoord * vec2(sprite.uScale, sprite.vScale) + vec2(sprite.u0, sprite.v0);
    fragColor = vec4(sprite.r, sprite.g, sprite.b, sprite.a);
}
