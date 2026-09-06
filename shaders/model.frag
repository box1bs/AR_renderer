#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec3 lightPos;
in vec2 uv;

out vec4 color;

uniform sampler2D tex;
uniform vec3 Ka, Ks, lightColor;
uniform float Ns;

void main() {
//    color = texture(tex, uv);
    vec3 N = normalize(normal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(-fragPos);
    vec3 H = normalize(L + V);

    vec3 diffuseColor = texture(tex, uv).rgb;
    vec3 ambient  = Ka * lightColor * 0.15;
    vec3 diffuse  = diffuseColor * lightColor * max(dot(N, L), 0.0);
    vec3 specular = Ks * lightColor * pow(max(dot(N, H), 0.0), Ns);

    color = vec4(ambient + diffuse + specular, 1.0);
}