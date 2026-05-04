#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform int useTexture;

void main()
{
    if (useTexture == 1) {
        vec4 color = texture(texture1, TexCoord);
        if (color.a < 0.1)
            discard;
        FragColor = color;
    } else {
        FragColor = vec4(0.92, 0.80, 0.67, 1.0);
    }
}
