#version 330

// Input
in vec3 frag_position;
in vec3 frag_color;
in vec2 frag_texture;
in vec3 frag_normal;
in float height;
flat in int frag_objectType;

// Output
layout(location = 0) out vec4 out_color;

// Function to calculate terrain color based on height
vec3 getTerrainColor(float height) {
    // Define colors for terrain gradient
    vec3 lowColor = vec3(0.2, 0.8, 0.3);
    vec3 highColor = vec3(1, 1, 1);
    return mix(lowColor, highColor, height);
}

void main()
{
    if (frag_objectType == 1) {
        // Black for the airscrew
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
    } else if (frag_objectType == 2) {
        // Light grey for the drone body
        out_color = vec4(0.8, 0.8, 0.8, 1.0);
    } else if (frag_objectType == 3) {
        // Brown for the tree trunk
        out_color = vec4(0.55, 0.27, 0.07, 1.0);
    } else if (frag_objectType == 4) {
        // Green for the tree foliage
        out_color = vec4(0.0, 0.5, 0.0, 1.0);
    } else if (frag_objectType == 5) {
        // Dark grey for the houses
        out_color = vec4(0.5, 0.5, 0.5, 1.0);
    } else if (frag_objectType == 6) {
        // Terrain coloring based on noise
        float heightFactor = clamp(height, 0.0, 1.0);
        vec3 terrainColor = getTerrainColor(heightFactor);
        out_color = vec4(terrainColor, 1.0);
    } else if (frag_objectType == 7) {
    out_color = vec4(0.996, 0.0, 0.0, 1.0);
    }
}
