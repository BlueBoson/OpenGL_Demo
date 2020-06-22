#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;
uniform float surfaceY;

void main()
{
	vec4 modPos = model * vec4(aPos, 1.0);
	float alpha = (surfaceY - modPos.y) / (modPos.y - lightPos.y);
	gl_Position = projection * view * vec4(modPos.xyz + (modPos.xyz - lightPos) * alpha, 1.0);
}