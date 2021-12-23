#version 450

//output variables have to be specifically defined 
//the location value specifies the index of the framebuffer that the variable is for
layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}