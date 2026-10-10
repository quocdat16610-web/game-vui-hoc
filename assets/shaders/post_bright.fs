#version 330
// Hậu kỳ bước 1: chỉ giữ lại các điểm SÁNG (nguồn của hiệu ứng bloom)
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float threshold;
out vec4 finalColor;

void main()
{
    vec3 c = texture(texture0, fragTexCoord).rgb;
    float l = max(c.r, max(c.g, c.b));
    float k = smoothstep(threshold, threshold + 0.2, l);
    finalColor = vec4(c * k, 1.0);
}
