#version 330
// =============================================================================
//  lit.fs — FRAGMENT SHADER: chạy cho MỖI ĐIỂM ẢNH của vật thể.
//  Tính ánh sáng: môi trường (hemisphere) + mặt trời (Lambert + Blinn-Phong)
//  + bóng đổ (shadow map, lọc PCF) + tối đa 8 đèn điểm + viền sáng + sương mù.
//  Thử sửa các con số trong file này rồi chạy lại game để xem thay đổi!
// =============================================================================
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3  viewPos;
uniform vec3  sunDir;          // hướng ánh nắng chiếu tới (đã chuẩn hóa)
uniform vec3  sunColor;        // màu * cường độ
uniform vec3  skyColor;        // ánh sáng môi trường từ trên
uniform vec3  groundColor;     // ánh sáng môi trường dội từ dưới đất
uniform vec3  fogColor;
uniform vec2  fogRange;        // (bắt đầu, kết thúc)
uniform float exposure;

#define MAX_LIGHTS 8
uniform int   lightCount;
uniform vec3  lightPos[MAX_LIGHTS];
uniform vec3  lightColor[MAX_LIGHTS];   // màu * cường độ
uniform float lightRange[MAX_LIGHTS];

uniform int       useShadow;
uniform mat4      lightVP;
uniform sampler2D shadowMap;
uniform float     shadowTexel;     // 1 / độ phân giải shadow map

out vec4 finalColor;

// Trả về 1.0 nếu được chiếu sáng hoàn toàn, 0.0 nếu nằm trong bóng
float ShadowFactor(vec3 n, vec3 L)
{
    vec4 p = lightVP * vec4(fragPosition + n * 0.02, 1.0);
    p.xyz /= p.w;
    p.xyz = p.xyz * 0.5 + 0.5;
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0) return 1.0;
    float bias = max(0.0012 * (1.0 - dot(n, L)), 0.0003);
    float lit = 0.0;
    for (int x = -1; x <= 1; x++)          // PCF: lấy mẫu 3x3 cho viền bóng mềm
        for (int y = -1; y <= 1; y++)
        {
            float d = texture(shadowMap, p.xy + vec2(x, y) * shadowTexel).r;
            lit += (p.z - bias > d) ? 0.0 : 1.0;
        }
    return lit / 9.0;
}

void main()
{
    vec4 texel = texture(texture0, fragTexCoord) * fragColor * colDiffuse;
    vec3 albedo = pow(texel.rgb, vec3(2.2));            // sRGB -> tuyến tính

    vec3 n = normalize(fragNormal);
    if (!gl_FrontFacing) n = -n;
    vec3 v = normalize(viewPos - fragPosition);
    vec3 L = -sunDir;

    // 1. Ánh sáng môi trường dạng bán cầu
    vec3 ambient = mix(groundColor, skyColor, n.y * 0.5 + 0.5);

    // 2. Mặt trời: Lambert (khuếch tán) + Blinn-Phong (bóng loáng)
    float ndl = max(dot(n, L), 0.0);
    float sh = (useShadow == 1) ? ShadowFactor(n, L) : 1.0;
    vec3 h = normalize(L + v);
    float spec = pow(max(dot(n, h), 0.0), 48.0) * 0.35 * ndl;
    vec3 color = albedo * (ambient + sunColor * ndl * sh) + sunColor * spec * sh;

    // 3. Đèn điểm (point light) với suy giảm theo khoảng cách
    for (int i = 0; i < lightCount; i++)
    {
        vec3 d = lightPos[i] - fragPosition;
        float dist = length(d);
        float att = clamp(1.0 - dist / lightRange[i], 0.0, 1.0);
        att *= att;
        vec3 ld = d / max(dist, 0.0001);
        float nl = max(dot(n, ld), 0.0);
        float s = pow(max(dot(n, normalize(ld + v)), 0.0), 32.0) * 0.4;
        color += (albedo * nl + s) * lightColor[i] * att;
    }

    // 4. Viền sáng (rim light) giúp nhân vật nổi khỏi nền
    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);
    color += rim * skyColor * 0.25;

    // 5. Sương mù theo khoảng cách
    float f = clamp((length(viewPos - fragPosition) - fogRange.x) / (fogRange.y - fogRange.x), 0.0, 1.0);
    color = mix(color, pow(fogColor, vec3(2.2)), f);

    // 6. Tone mapping (ACES gần đúng) rồi đổi lại sRGB
    color *= exposure;
    color = clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
    finalColor = vec4(pow(color, vec3(1.0 / 2.2)), texel.a);
}
