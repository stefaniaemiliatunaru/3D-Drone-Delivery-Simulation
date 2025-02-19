#version 330

// Input
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture;
layout(location = 3) in vec3 v_color;

// Uniform properties
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform int objectType;

// Output
out vec3 frag_position;
out vec3 frag_color;
out vec2 frag_texture;
out vec3 frag_normal;
out float height;
flat out int frag_objectType;

float random (in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{
	float amplitude = 0.8;
    float frequency = 0.4;
    // Calculate noise-based height
    vec2 noiseInput = v_position.xz * frequency;
    height = noise(noiseInput) * amplitude;
    vec3 adjustedPosition = v_position;
    adjustedPosition.y += height;
    frag_position = vec3(Model * vec4(adjustedPosition, 1.0));
    frag_normal = mat3(transpose(inverse(Model))) * v_normal;
    frag_texture = v_texture;
    frag_objectType = objectType;
    frag_color = mix(vec3(0.2, 0.8, 0.3), vec3(0.5, 0.3, 0.1), height / amplitude);

    gl_Position = Projection * View * vec4(frag_position, 1.0);
}
