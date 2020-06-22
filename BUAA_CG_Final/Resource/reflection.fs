#version 330 core

out vec4 FragColor;

in vec3 Normal;
in vec3 Position;
in vec3 myPosition;

uniform vec3 cameraPos;
uniform vec3 colour;
uniform samplerCube skybox;

void main()
{    
    vec3 I = normalize(Position - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(mix(texture(skybox, R).rgb, myPosition, 0.3), 1.0);
}