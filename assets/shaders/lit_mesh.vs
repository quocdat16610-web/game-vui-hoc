#version 330
// =============================================================================
//  lit_mesh.vs — vertex shader cho MODEL tải từ file (glTF): khác lit.vs ở chỗ
//  đỉnh còn ở không gian model, phải nhân ma trận matModel để ra thế giới.
// =============================================================================
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal   = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragColor    = vertexColor;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
