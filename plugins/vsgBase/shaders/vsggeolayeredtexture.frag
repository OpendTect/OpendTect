#version 450

layout(set = 0, binding = 0) uniform sampler2D indexMap;
layout(set = 0, binding = 1) uniform sampler2D colorMap;

layout(location = 0) in vec2 texCoord0;

layout(location = 0) out vec4 outColor;

// Must be the same as dGBVis::ColTabMapping::nrSteps(): the number of colors
// the indices point to. The color strip has a pixel for each of them, and
// its last pixel is the undefined color.
const float nrSteps = 255.0;

// The first band is the index multiplied by the second one, which is 1 for a
// defined cell and 0 for an undefined one. After interpolation their ratio is
// the average index of the defined cells around the point.
void main()
{
    vec2 bands = texture( indexMap, texCoord0 ).rg;
    float width = float( textureSize(colorMap, 0).x );
    float pos = bands.g < 0.5
	? width - 0.5
	: clamp( bands.r / bands.g * 255.0, 0.0, nrSteps - 1.0 ) + 0.5;
    outColor = texture( colorMap, vec2(pos / width, 0.5) );
}
