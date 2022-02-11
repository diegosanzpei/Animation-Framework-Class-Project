#version 440 core

out vec4 out_color;
layout(location = 2) uniform vec4 u_color;
layout(location = 3) uniform bool u_use_texture;
layout(location = 4) uniform vec3 u_light_dir;
layout(location = 5) uniform vec3 u_light_color;
layout(location = 6) uniform vec3 u_cam_pos;
layout(location = 7) uniform bool u_no_normals;
layout(location = 8) uniform float u_ambient;
layout(location = 9) uniform float u_specular;
layout(location = 10) uniform bool u_use_normal_map;

// Textures
uniform sampler2D diffuse;
uniform sampler2D normal_map;

in vec2 diffuse_coord;
in vec2 normal_map_coord;
in vec3 normal;
in vec3 frag_pos;
in mat3 TBN;

vec4 get_object_color()
{
    if (u_use_texture == false)
        return u_color;
    else
        return texture(diffuse, diffuse_coord);
}

vec3 compute_normal()
{
    if (u_use_normal_map)
    {
        vec3 norm = texture(normal_map, normal_map_coord).rgb;
        norm = norm * 2.0 - 1.0;
        return normalize(TBN * norm);
    }
    
    else
        return normalize(normal);
}

vec4 lighting()
{
    vec3 ambient = u_ambient * u_light_color;
    
    vec3 norm = compute_normal();
    vec3 light_dir = normalize(-u_light_dir);
    
    float diff = max(dot(norm, light_dir), 0.0f);
    vec3 diffuse = diff * u_light_color;
    
    vec3 view_dir = normalize(u_cam_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm); 
    
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = u_specular * spec * u_light_color;
    
    vec3 object_color = vec3(get_object_color());

    return vec4((specular + ambient + diffuse) * object_color, 1.0f);
}

void main()
{
    // Use lighting
    if (u_no_normals == false)
        out_color = lighting();

     // No lighting
     else
         out_color = get_object_color();
}
