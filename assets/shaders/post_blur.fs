#version 330
// Hậu kỳ bước 2: làm mờ Gauss 9 điểm theo 1 hướng (chạy 2 lần: ngang rồi dọc)
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 direction;      // (1/rộng, 0) hoặc (0, 1/cao)
out vec4 finalColor;

void main()
{
    float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 c = texture(texture0, fragTexCoord).rgb * w[0];
    for (int i = 1; i < 5; i++)
    {
        vec2 o = direction * float(i) * 1.5;
        c += texture(texture0, fragTexCoord + o).rgb * w[i];
        c += texture(texture0, fragTexCoord - o).rgb * w[i];
    }
    finalColor = vec4(c, 1.0);
}
