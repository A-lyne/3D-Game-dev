#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform sampler2D diffuseTex;
uniform sampler2D cloudsTex;
uniform sampler2D nightTex;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Sample the diffuse texture
    vec3 diffuseColor = texture(diffuseTex, TexCoord).rgb;
    
    // Simple lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Ambient + diffuse lighting
    vec3 ambient = 0.3 * lightColor;
    vec3 diffuse = diff * lightColor;
    
    vec3 result = (ambient + diffuse) * diffuseColor;
    FragColor = vec4(result, 1.0);
}
