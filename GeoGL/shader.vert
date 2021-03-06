#version 120

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

in vec3 in_Position;
in vec3 in_Color;

out vec3 pass_Color;

void main(void)
{
     gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(in_Position, 1.0);
	 /*gl_Position = projectionMatrix * modelMatrix * vec4(in_Position, 1.0);*/
     pass_Color = in_Color;
	 /*pass_Color = vec3(1.0,1.0,1.0);*/
}