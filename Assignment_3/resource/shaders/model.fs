#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture_diffuse1;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform bool useTexture;
uniform vec3 objectColor;

void main()
{
    vec3 color;
    if (useTexture) {
        color = texture(texture_diffuse1, TexCoord).rgb;
    } else {
        color = objectColor;
    }
    
    // Ambient
    vec3 ambient = 0.3 * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
}

