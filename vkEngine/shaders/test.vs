#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_texCoords;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	float time;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texCoords;
layout(location = 2) out float out_time;

void main()
{
	gl_Position = vec4(in_position, 1.0);
	out_texCoords = in_texCoords;
	out_color = in_color;
	out_time = ubo.time;
}