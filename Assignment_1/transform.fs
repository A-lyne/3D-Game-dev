#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
    // Use mario.png texture on all faces
    FragColor = texture(texture1, TexCoord);
}

