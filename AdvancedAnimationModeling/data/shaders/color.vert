#version 440 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec2 a_normal_map_uv;
layout(location = 4) in vec3 a_tangent;
layout(location = 5) in vec4 a_joints;
layout(location = 6) in vec4 a_weights;

layout(location = 0)  uniform mat4 u_mvp;
layout(location = 1)  uniform mat4 u_model;
layout(location = 3)  uniform bool u_use_texture;
layout(location = 7)  uniform bool u_no_normals;
layout(location = 10) uniform bool u_use_normal_map;
layout(location = 11) uniform bool u_skinned;
layout(location = 12) uniform mat4 u_joint_matrices[128];

out vec2 diffuse_coord;
out vec2 normal_map_coord;
out vec3 normal;
out vec3 frag_pos;
out mat3 TBN;

void compute_normal()
{
    // Fragment position in world space
    frag_pos = vec3(u_model * vec4(a_pos, 1.0f));

    // Compute TBN matrix for normal mapping
    if (u_use_normal_map)
    {
        vec3 T = normalize(vec3(u_model * vec4(a_tangent, 0.0)));
        vec3 bitangent = cross(a_normal, a_tangent);
        vec3 B = normalize(vec3(u_model * vec4(bitangent, 0.0)));
        vec3 N = normalize(vec3(u_model * vec4(a_normal,  0.0)));
        TBN = mat3(T, B, N);
        normal_map_coord = a_normal_map_uv;
    }

    else
        normal = mat3(transpose(inverse(u_model))) * a_normal;
}

mat4 skin_mtx()
{
    mat4 skinMatrix =
        a_weights.x * u_joint_matrices[int(a_joints.x)] +
        a_weights.y * u_joint_matrices[int(a_joints.y)] +
        a_weights.z * u_joint_matrices[int(a_joints.z)] +
        a_weights.w * u_joint_matrices[int(a_joints.w)];

    return skinMatrix;
}

void main()
{
    if (u_no_normals == false)
        compute_normal();
    
    if (u_use_texture)
        diffuse_coord = a_uv;

    vec4 vertex = vec4(a_pos, 1.0f);
    
    if (u_skinned)
        gl_Position = u_mvp * skin_mtx() * vertex;
    else
        gl_Position = u_mvp * vertex;

}