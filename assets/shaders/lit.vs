#version 330
// =============================================================================
//  lit.vs — VERTEX SHADER: chạy 1 lần cho MỖI ĐỈNH (vertex) trên GPU.
//  raylib đã nhân sẵn ma trận biến đổi (rlPushMatrix/rlTranslatef...) trên CPU
//  nên vertexPosition ở đây đã là tọa độ THẾ GIỚI.
// =============================================================================
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;          // view * projection của camera (raylib tự gửi)

out vec3 fragPosition;     // gửi sang fragment shader (được nội suy)
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    fragPosition = vertexPosition;
    fragTexCoord = vertexTexCoord;
    fragNormal   = vertexNormal;
    fragColor    = vertexColor;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
