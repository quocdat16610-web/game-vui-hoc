#version 330
// Hậu kỳ bước 3: ghép ảnh cảnh + bloom, khử răng cưa FXAA, chỉnh màu, vignette
in vec2 fragTexCoord;
uniform sampler2D texture0;      // ảnh cảnh 3D
uniform sampler2D bloomTex;      // ảnh sáng đã làm mờ
uniform vec2  resolution;
uniform float bloomStrength;
uniform float vignette;
uniform float saturation;
uniform float contrast;
uniform float aberration;        // tách màu khi trúng đòn mạnh
uniform float flash;             // chớp trắng
uniform float time;
out vec4 finalColor;

vec3 Fxaa(vec2 uv)
{
    vec2 rcp = 1.0 / resolution;
    vec3 rgbNW = texture(texture0, uv + vec2(-1.0, -1.0) * rcp).rgb;
    vec3 rgbNE = texture(texture0, uv + vec2( 1.0, -1.0) * rcp).rgb;
    vec3 rgbSW = texture(texture0, uv + vec2(-1.0,  1.0) * rcp).rgb;
    vec3 rgbSE = texture(texture0, uv + vec2( 1.0,  1.0) * rcp).rgb;
    vec3 rgbM  = texture(texture0, uv).rgb;
    vec3 lw = vec3(0.299, 0.587, 0.114);
    float lNW = dot(rgbNW, lw), lNE = dot(rgbNE, lw), lSW = dot(rgbSW, lw), lSE = dot(rgbSE, lw), lM = dot(rgbM, lw);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    vec2 dir = vec2(-((lNW + lNE) - (lSW + lSE)), ((lNW + lSW) - (lNE + lSE)));
    float reduce = max((lNW + lNE + lSW + lSE) * 0.03125, 1.0 / 128.0);
    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * rcpMin, vec2(-8.0), vec2(8.0)) * rcp;
    vec3 a = 0.5 * (texture(texture0, uv + dir * (1.0 / 3.0 - 0.5)).rgb + texture(texture0, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 b = a * 0.5 + 0.25 * (texture(texture0, uv - dir * 0.5).rgb + texture(texture0, uv + dir * 0.5).rgb);
    float lB = dot(b, lw);
    return (lB < lMin || lB > lMax) ? a : b;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec3 c = Fxaa(uv);
    if (aberration > 0.0)
    {
        vec2 off = (uv - 0.5) * aberration * 0.02;
        c.r = texture(texture0, uv + off).r;
        c.b = texture(texture0, uv - off).b;
    }
    c += texture(bloomTex, uv).rgb * bloomStrength;

    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, saturation);                 // độ bão hòa màu
    c = (c - 0.5) * contrast + 0.5;                  // độ tương phản

    vec2 d = uv - 0.5;
    c *= 1.0 - dot(d, d) * vignette * 2.0;           // tối dần ở góc màn hình

    float grain = fract(sin(dot(uv * resolution + time, vec2(12.9898, 78.233))) * 43758.5453);
    c += (grain - 0.5) * 0.015;                      // hạt nhiễu nhẹ kiểu phim

    c = mix(c, vec3(1.0), flash);
    finalColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
