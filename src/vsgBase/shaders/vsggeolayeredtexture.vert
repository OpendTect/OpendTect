#version 450

layout(push_constant) uniform PushConstants
{
    mat4 projection;
    mat4 modelView;
} pc;

layout(location = 0) in vec3 vsg_Vertex;
layout(location = 1) in vec2 vsg_TexCoord0;

layout(location = 0) out vec2 texCoord0;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = (pc.projection * pc.modelView) * vec4(vsg_Vertex, 1.0);
    texCoord0 = vsg_TexCoord0;
}
